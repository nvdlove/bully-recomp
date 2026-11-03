// MDXact's manager (0x5AC870..0x5ADD6B, 0x5AE2A0..0x5AE9B6): XACT's start-up, the maps
// from the game's handles to the XACT objects and their shared data (under the critical
// section 0xCFAD28), reference counts on the XACT banks and cues, the cue-destroyed
// notification, the per-frame update, and the command handlers (loading and unloading
// banks, preparing cues, and forwarding cue calls by handle). See MDXactCommands.cpp.
//
// The maps (NiTPointerMap: +4 buckets, +8 bucket array, items {next, key, value}):
// 0xCFACCC wave bank data, 0xCFACD0 sound bank data, 0xCFACD4 cue data (by handle),
// 0xCFACE4 wave banks, 0xCFACE8 sound banks, 0xCFACEC cues (by handle), 0xCFACF0/F4/F8
// the XACT wave banks', sound banks' and cues' reference counts (by pointer).

#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

using re::At;

struct MapItem {   // (the game's layout: 0xC)
    re::Ptr32<MapItem> next;
    uint32_t key;
    re::Ptr32<void> value;
};
static_assert(sizeof(MapItem) == 0xC);

struct PtrMap {
    re::Ptr32<void> vtable;
    uint32_t size;
    re::Ptr32<re::Ptr32<MapItem>> buckets;
    uint32_t count;
};

GAME_VAR(void*, xactInitialised, 0x00CFACC0);
GAME_VAR(uint8_t*, pendingCommands, 0x00CFACC4);
GAME_VAR(int32_t, pendingCommandsTail, 0x00CFACC8);
GAME_VAR(PtrMap*, waveBankData, 0x00CFACCC);
GAME_VAR(PtrMap*, soundBankData, 0x00CFACD0);
GAME_VAR(PtrMap*, cueData, 0x00CFACD4);
GAME_VAR(uint8_t*, xactEngine, 0x00CFACE0);
GAME_VAR(PtrMap*, waveBanks, 0x00CFACE4);
GAME_VAR(PtrMap*, soundBanks, 0x00CFACE8);
GAME_VAR(PtrMap*, cues, 0x00CFACEC);
GAME_VAR(PtrMap*, waveBankRefs, 0x00CFACF0);
GAME_VAR(PtrMap*, soundBankRefs, 0x00CFACF4);
GAME_VAR(PtrMap*, cueRefs, 0x00CFACF8);
GAME_VAR(void*, xactSettings, 0x00CFACFC);
GAME_VAR(uint32_t, xactSettingsSize, 0x00CFAD00);
GAME_VAR(int32_t, xactReverbType, 0x00CFAD04);
GAME_VAR(uint8_t, xactReverbForced, 0x00CFAD08);
GAME_VAR(uint8_t, xactUpdating, 0x00CFAD09);
GAME_VAR(void*, destroyingCue, 0x00CFAD0C);
inline void* const commandSection = reinterpret_cast<void*>(0x00CFAD10);
inline void* const dataSection = reinterpret_cast<void*>(0x00CFAD28);
inline const re::Ptr32<const char>* const emptyFixedString = reinterpret_cast<const re::Ptr32<const char>*>(0x00951028);

using SectionFn = void(__stdcall*)(void*);
inline void Lock(void* section) {
    re::GameFnAt<SectionFn>(uintptr_t(0x008FC17C))(section);
}
inline void Unlock(void* section) {
    re::GameFnAt<SectionFn>(uintptr_t(0x008FC178))(section);
}

GAME_FN(void*, __cdecl, New, 0x005EEAA0, uint32_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
inline void* __cdecl Memcpy(void* dst, const void* src, uint32_t size) { return crt::Memcpy(dst, src, (size_t)size); }
inline void* __cdecl Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
GAME_FN(void*, __cdecl, NiNew, 0x00752460, uint32_t size);
GAME_FN(long, __stdcall, XactCreateEngine, 0x00404F60, uint32_t flags, re::Ptr32<uint8_t>* engine);
GAME_FN(void, __thiscall, Map_SetAt, 0x008A5140, void* map, uint32_t key, void* value);
GAME_FN(uint8_t, __thiscall, Map_RemoveAt, 0x008A5A10, void* map, uint32_t key);
GAME_FN(void*, __thiscall, Map_GetFirstPos, 0x008CA4E0, void* map);
GAME_FN(void, __thiscall, Map_GetNext, 0x008976C0, void* map, re::Ptr32<void>* position, uint32_t* key, re::Ptr32<void>* value);
GAME_FN(const char*, __cdecl, NiFixedString_Create, 0x00754760, const char* text);
GAME_FN(void, __thiscall, NiFixedString_Destruct, 0x00517750, re::Ptr32<const char>* self);
GAME_FN(void, __thiscall, NiStringTable_Free, 0x00754550, void* table, re::Ptr32<const char>* string, uint32_t length);
GAME_FN(void, __thiscall, EmptyThiscall, 0x0049A650, void* self);
GAME_FN(uint8_t, __thiscall, Map_GetAt, 0x005AC5F0, void* map, uint32_t key, re::Ptr32<void>* value);
GAME_FN(void, __cdecl, Voice_SetLevel, 0x005A8050, void* voice, int level);
GAME_FN(void, __cdecl, Speakers_PanRotated, 0x005A8C60, void* voice, int level, int angle);
GAME_FN(void, __cdecl, Speakers_PanByAngle, 0x005A8460, void* voice, int level, int angle, float volume);
GAME_FN(void, __cdecl, Speakers_Pan51, 0x005A8620, void* voice, int level, int angle, float volume);
GAME_FN(void, __cdecl, Voice_Silence, 0x005A7E20, void* voice);
GAME_FN(void, __thiscall, Voice_SetGains, 0x005AEAE0, void* voice, float, float, float, float, float, float);
// the map constructors (MDXactCommands.cpp)
GAME_FN(void*, __thiscall, Map_Construct0, 0x005ADD70, void* self, uint32_t size);
GAME_FN(void*, __thiscall, Map_Construct1, 0x005ADDC0, void* self, uint32_t size);
GAME_FN(void*, __thiscall, Map_Construct2, 0x005ADE10, void* self, uint32_t size);
GAME_FN(void*, __thiscall, Map_Construct3, 0x005ADE60, void* self, uint32_t size);
GAME_FN(void*, __thiscall, Map_Construct4, 0x005ADEB0, void* self, uint32_t size);
GAME_FN(void*, __thiscall, Map_Construct5, 0x005ADF00, void* self, uint32_t size);
GAME_FN(void*, __thiscall, Map_Construct6, 0x005ADF70, void* self, uint32_t size);
GAME_FN(void*, __thiscall, Map_Construct7, 0x005ADFC0, void* self, uint32_t size);
GAME_FN(void*, __thiscall, Map_Construct8, 0x005AE010, void* self, uint32_t size);
// the objects (MDXactObjects.cpp)
GAME_FN(void, __thiscall, Cue_Destruct, 0x005AEEC0, void* cue);
GAME_FN(void, __thiscall, Cue_GetState, 0x005AEA10, void* cue, uint32_t* state);
GAME_FN(void, __thiscall, Cue_Pause, 0x005AEA30, void* cue, int pause);
GAME_FN(void, __thiscall, Cue_SetVariableB, 0x005AEB50, void* cue, float value);
GAME_FN(void, __thiscall, Cue_SetVariableA, 0x005AEB10, void* cue, float value);
GAME_FN(void, __thiscall, Cue_SetReverbMix, 0x005AEAD0, void* cue, float value);
GAME_FN(uint8_t, __thiscall, Cue_GetProperties, 0x005AEB90, void* cue, uint8_t* out);
GAME_FN(int32_t, __thiscall, Cue_Play, 0x005AF060, void* cue, int32_t fadeIn, int32_t fadeOut);
GAME_FN(void, __thiscall, SoundBank_StopCue, 0x005AFAA0, void* bank, void* cue, int32_t flags);
GAME_FN(void, __thiscall, SoundBank_PauseCues, 0x005AFB20, void* bank);
GAME_FN(void, __thiscall, SoundBank_ResumeCues, 0x005AFBE0, void* bank);
GAME_FN(re::Ptr32<const char>*, __thiscall, SoundBank_GetCueName, 0x005AFCE0, void* bank, re::Ptr32<const char>* out, int32_t index);
GAME_FN(void, __thiscall, SoundBank_Tidy, 0x005AFD80, void* bank);
GAME_FN(void, __thiscall, SoundBank_DestroyCue, 0x005AFDB0, void* bank, void* cue);
GAME_FN(void, __thiscall, SoundBank_RemoveCue, 0x005AFE80, void* bank, void* cue);
GAME_FN(void, __thiscall, SoundBank_StopCueIfDone, 0x005AFF10, void* bank, void* cue, int32_t flags);
GAME_FN(void, __thiscall, SoundBank_Update, 0x005AFF50, void* bank, uint32_t now);
GAME_FN(uint8_t, __thiscall, SoundBank_Release, 0x005B0090, void* bank);
GAME_FN(void*, __thiscall, SoundBank_Construct, 0x005B04E0, void* self, void* xactBank, int32_t handle);
GAME_FN(uint8_t*, __thiscall, SoundBank_PrepareCue, 0x005B0570, void* bank, re::Ptr32<const char>* name, int index);
GAME_FN(uint8_t*, __thiscall, SoundBank_PrepareCueByName, 0x005B0630, void* bank, re::Ptr32<const char>* name);
GAME_FN(void*, __thiscall, WaveBank_Construct, 0x005B0660, void* self, void* xactBank, int streaming, int32_t handle);
GAME_FN(uint8_t, __thiscall, WaveBank_Release, 0x005B0680, void* bank);
GAME_FN(void, __thiscall, WaveBank_GetState, 0x005B06A0, void* bank, uint32_t* state);
// this file's
GAME_FN(void, __cdecl, Data_AddWaveBank, 0x005AC9C0, int32_t handle);
GAME_FN(void, __cdecl, Data_AddSoundBank, 0x005ACA10, int32_t handle);
GAME_FN(void, __cdecl, Data_AddCue, 0x005ACA50, int32_t handle);
GAME_FN(void, __cdecl, Data_MarkWaveBank, 0x005AD4D0, int32_t handle);
GAME_FN(void, __cdecl, Data_MarkSoundBank, 0x005AD530, int32_t handle);
GAME_FN(void, __cdecl, Data_MarkCue, 0x005AD590, int32_t handle);
GAME_FN(void, __cdecl, Data_RemoveWaveBank, 0x005AD5F0, int32_t handle);
GAME_FN(void, __cdecl, Data_RemoveSoundBank, 0x005AD660, int32_t handle);
GAME_FN(void, __cdecl, Data_RemoveCue, 0x005AD6D0, int32_t handle);
GAME_FN(void, __cdecl, Data_SetWaveBankState, 0x005AD740, int32_t handle, uint32_t state);
GAME_FN(void, __cdecl, Data_SetCueState, 0x005AD7A0, int32_t handle, uint32_t state, uint32_t channels);
GAME_FN(void, __cdecl, Ref_AddWaveBank, 0x005AD810, void* bank);
GAME_FN(void, __cdecl, Ref_AddSoundBank, 0x005AD8E0, void* bank);
GAME_FN(void, __cdecl, Ref_ReleaseSoundBank, 0x005AD950, void* bank);
GAME_FN(void, __cdecl, Ref_ReleaseCue, 0x005ADA20, void* cue);
GAME_FN(void, __cdecl, Cues_Remove, 0x005AE990, void* cue);

MapItem* Find(PtrMap* map, uint32_t key) {
    for (MapItem* item = map->buckets[key % map->size]; item != nullptr; item = item->next) {
        if (item->key == key) {
            return item;
        }
    }
    return nullptr;
}
inline uint32_t Key(const void* p) {
    return uint32_t(reinterpret_cast<uintptr_t>(p));
}

// NiTMap's first item and the next one (the next bucket's first when the chain ends)
MapItem* FirstItem(PtrMap* map) {
    for (uint32_t i = 0; i < map->size; ++i) {
        if (map->buckets[i] != nullptr) {
            return map->buckets[i];
        }
    }
    return nullptr;
}
MapItem* NextItem(PtrMap* map, MapItem* item) {
    if (item->next != nullptr) {
        return item->next;
    }
    for (uint32_t i = item->key % map->size + 1; i < map->size; ++i) {
        if (map->buckets[i] != nullptr) {
            return map->buckets[i];
        }
    }
    return nullptr;
}

// ---- the shared data (by handle, under 0xCFAD28) -------------------------------------------

// 0x5AC870: a wave bank's state
int32_t __cdecl Data_WaveBankState(uint32_t handle) {
    Lock(dataSection);
    MapItem* item = Find(waveBankData, handle);
    if (item == nullptr || item->value == nullptr) {
        Unlock(dataSection);
        return 1;
    }
    if (At<uint8_t>(item->value, 4) != 0) {
        Unlock(dataSection);
        return 2;
    }
    Unlock(dataSection);
    return 0;
}

// 0x5AC8E0: a sound bank's state
int32_t __cdecl Data_SoundBankState(uint32_t handle) {
    Lock(dataSection);
    MapItem* item = Find(soundBankData, handle);
    if (item == nullptr || item->value == nullptr) {
        Unlock(dataSection);
        return 1;
    }
    if (At<uint8_t>(item->value, 4) != 0) {
        Unlock(dataSection);
        return 2;
    }
    Unlock(dataSection);
    return 0;
}

// 0x5AC950: a cue's state
int32_t __cdecl Data_CueState(uint32_t handle) {
    Lock(dataSection);
    MapItem* item = Find(cueData, handle);
    if (item == nullptr || item->value == nullptr) {
        Unlock(dataSection);
        return 1;
    }
    if (At<uint8_t>(item->value, 4) != 0) {
        Unlock(dataSection);
        return 2;
    }
    Unlock(dataSection);
    return 0;
}

// 0x5AC9C0: a wave bank's data ({handle, loaded, state}, 0xC bytes)
void __cdecl Data_AddWaveBankImpl(uint32_t handle) {
    Lock(dataSection);
    uint8_t* data = static_cast<uint8_t*>(New(0xC));
    if (data != nullptr) {
        At<uint32_t>(data, 0) = handle;
        At<uint8_t>(data, 4) = 0;
        At<uint32_t>(data, 8) = 0;
        Map_SetAt(waveBankData, handle, data);
    }
    Unlock(dataSection);
}

// 0x5ACA10: a sound bank's data ({handle, loaded}, 8 bytes)
void __cdecl Data_AddSoundBankImpl(uint32_t handle) {
    Lock(dataSection);
    uint8_t* data = static_cast<uint8_t*>(New(8));
    if (data != nullptr) {
        At<uint32_t>(data, 0) = handle;
        At<uint8_t>(data, 4) = 0;
        Map_SetAt(soundBankData, handle, data);
    }
    Unlock(dataSection);
}

// 0x5ACA50: a cue's data ({handle, loaded, state, channels, paused, speakers}, 0x14 bytes)
void __cdecl Data_AddCueImpl(uint32_t handle) {
    Lock(dataSection);
    uint8_t* data = static_cast<uint8_t*>(New(0x14));
    if (data != nullptr) {
        At<uint32_t>(data, 0) = handle;
        At<uint8_t>(data, 4) = 0;
        At<uint32_t>(data, 8) = 0;
        At<uint32_t>(data, 0xC) = 0;
        At<uint8_t>(data, 0x10) = 0;
        At<uint8_t>(data, 0x11) = 0;
        Map_SetAt(cueData, handle, data);
    }
    Unlock(dataSection);
}

// 0x5ACAA0: a cue's paused flag (+0x10)
void __cdecl Data_SetCuePaused(uint32_t handle, int paused) {
    Lock(dataSection);
    MapItem* item = Find(cueData, handle);
    if (item != nullptr && item->value != nullptr) {
        At<uint8_t>(item->value, 0x10) = uint8_t(paused);
    }
    Unlock(dataSection);
}

// 0x5ACB00: a cue's speakers flag (+0x11)
void __cdecl Data_SetCueSpeakers(uint32_t handle, int speakers) {
    Lock(dataSection);
    MapItem* item = Find(cueData, handle);
    if (item != nullptr && item->value != nullptr) {
        At<uint8_t>(item->value, 0x11) = uint8_t(speakers);
    }
    Unlock(dataSection);
}

// 0x5ACB60: a loaded wave bank's state (+8)
uint8_t __cdecl Data_GetWaveBankState(uint32_t handle, uint32_t* state) {
    Lock(dataSection);
    *state = 0;
    MapItem* item = Find(waveBankData, handle);
    if (item == nullptr || item->value == nullptr || At<uint8_t>(item->value, 4) == 0) {
        Unlock(dataSection);
        return 0;
    }
    *state = At<uint32_t>(item->value, 8);
    Unlock(dataSection);
    return 1;
}

// 0x5ACBD0: a cue's paused flag
uint8_t __cdecl Data_GetCuePaused(uint32_t handle, uint8_t* paused) {
    Lock(dataSection);
    *paused = 0;
    MapItem* item = Find(cueData, handle);
    if (item == nullptr || item->value == nullptr) {
        Unlock(dataSection);
        return 0;
    }
    *paused = At<uint8_t>(item->value, 0x10);
    Unlock(dataSection);
    return 1;
}

// 0x5ACC40: a cue's speakers flag
uint8_t __cdecl Data_GetCueSpeakers(uint32_t handle, uint8_t* speakers) {
    Lock(dataSection);
    *speakers = 0;
    MapItem* item = Find(cueData, handle);
    if (item == nullptr || item->value == nullptr) {
        Unlock(dataSection);
        return 0;
    }
    *speakers = At<uint8_t>(item->value, 0x11);
    Unlock(dataSection);
    return 1;
}

// 0x5ACCB0: a loaded cue's state (+8)
uint8_t __cdecl Data_GetCueState(uint32_t handle, uint32_t* state) {
    Lock(dataSection);
    *state = 0;
    MapItem* item = Find(cueData, handle);
    if (item == nullptr || item->value == nullptr || At<uint8_t>(item->value, 4) == 0) {
        Unlock(dataSection);
        return 0;
    }
    *state = At<uint32_t>(item->value, 8);
    Unlock(dataSection);
    return 1;
}

// 0x5ACD20: a loaded cue's channels (+0xC)
uint8_t __cdecl Data_GetCueChannels(uint32_t handle, uint32_t* channels) {
    Lock(dataSection);
    *channels = 0;
    MapItem* item = Find(cueData, handle);
    if (item == nullptr || item->value == nullptr || At<uint8_t>(item->value, 4) == 0) {
        Unlock(dataSection);
        return 0;
    }
    *channels = At<uint32_t>(item->value, 0xC);
    Unlock(dataSection);
    return 1;
}

// ---- the notification ---------------------------------------------------------------------------

// 0x5ACD90: XACT's notification (a cue destroyed: its pointer at +0xF): drops its reference,
// then every game cue using it (their data, their sound banks' lists, the cue map)
void __stdcall Xact_OnNotification(uint8_t* notification) {
    void* xactCue = *reinterpret_cast<re::Ptr32<void>*>(notification + 0xF);
    if (destroyingCue == xactCue) {
        return;
    }
    PtrMap* refs = cueRefs;
    MapItem* ref = Find(refs, Key(xactCue));
    if (ref != nullptr && ref->value != nullptr) {
        void* count = ref->value;
        Map_RemoveAt(refs, Key(xactCue));
        Free(count);
    }
restart:
    PtrMap* map = cues;
    for (MapItem* item = FirstItem(map); item != nullptr;) {
        MapItem* next = NextItem(map, item);
        uint8_t* cue = static_cast<uint8_t*>(item->value);
        if (cue != nullptr && At<void*>(cue, 0) == *reinterpret_cast<re::Ptr32<void>*>(notification + 0xF)) {
            At<void*>(cue, 0) = nullptr;
            Lock(dataSection);
            uint32_t handle = At<uint32_t>(cue, 4);
            PtrMap* data = cueData;
            MapItem* entry = Find(data, handle);
            if (entry != nullptr && entry->value != nullptr) {
                void* value = entry->value;
                Map_RemoveAt(data, handle);
                Free(value);
            }
            Unlock(dataSection);
            PtrMap* banks = soundBanks;
            for (MapItem* bank = FirstItem(banks); bank != nullptr;) {
                MapItem* nextBank = NextItem(banks, bank);
                if (bank->value != nullptr) {
                    SoundBank_RemoveCue(bank->value, cue);
                    banks = soundBanks;
                }
                bank = nextBank;
            }
            Map_RemoveAt(cues, At<uint32_t>(cue, 4));
            Cue_Destruct(cue);
            Free(cue);
            goto restart;
        }
        item = next;
    }
}

// ---- calls by handle ---------------------------------------------------------------------------

void* ValueOf(PtrMap* map, uint32_t key) {
    MapItem* item = Find(map, key);
    return item != nullptr ? item->value : nullptr;
}

// 0x5ACF70: stops a sound bank's cue
void __cdecl Xact_StopCueOf(uint32_t bankHandle, uint32_t cueHandle, int32_t flags) {
    void* bank = ValueOf(soundBanks, bankHandle);
    if (bank == nullptr) {
        return;
    }
    re::Ptr32<void> cue = nullptr;
    if (Map_GetAt(cues, cueHandle, &cue) != 0 && cue != nullptr) {
        SoundBank_StopCue(bank, cue, flags);
    }
}

// 0x5ACFE0: destroys a sound bank's cue
void __cdecl Xact_DestroyCueOf(uint32_t bankHandle, uint32_t cueHandle) {
    void* bank = ValueOf(soundBanks, bankHandle);
    if (bank == nullptr) {
        return;
    }
    re::Ptr32<void> cue = nullptr;
    if (Map_GetAt(cues, cueHandle, &cue) != 0 && cue != nullptr) {
        SoundBank_DestroyCue(bank, cue);
    }
}

// 0x5AD050: stops a sound bank's cue, destroying it unless it fades
void __cdecl Xact_StopCueIfDone(uint32_t bankHandle, uint32_t cueHandle, int32_t flags) {
    void* bank = ValueOf(soundBanks, bankHandle);
    if (bank == nullptr) {
        return;
    }
    re::Ptr32<void> cue = nullptr;
    if (Map_GetAt(cues, cueHandle, &cue) != 0 && cue != nullptr) {
        SoundBank_StopCueIfDone(bank, cue, flags);
    }
}

// 0x5AD0C0: plays a cue with fades
void __cdecl Xact_PlayCue(uint32_t cueHandle, int32_t fadeIn, int32_t fadeOut) {
    void* cue = ValueOf(cues, cueHandle);
    if (cue != nullptr) {
        Cue_Play(cue, fadeIn, fadeOut);
    }
}

// 0x5AD110: pauses a cue
void __cdecl Xact_PauseCue(uint32_t cueHandle, int pause) {
    void* cue = ValueOf(cues, cueHandle);
    if (cue != nullptr) {
        Cue_Pause(cue, pause);
    }
}

// 0x5AD150
void __cdecl Xact_SetCueVariableB(uint32_t cueHandle, float value) {
    void* cue = ValueOf(cues, cueHandle);
    if (cue != nullptr) {
        Cue_SetVariableB(cue, value);
    }
}

// 0x5AD1A0
void __cdecl Xact_SetCueVariableA(uint32_t cueHandle, float value) {
    void* cue = ValueOf(cues, cueHandle);
    if (cue != nullptr) {
        Cue_SetVariableA(cue, value);
    }
}

// 0x5AD1F0
void __cdecl Xact_SetCueReverbMix(uint32_t cueHandle, float value) {
    void* cue = ValueOf(cues, cueHandle);
    if (cue != nullptr) {
        Cue_SetReverbMix(cue, value);
    }
}

// 0x5AD240: a cue's speaker set (+0x18; +0x14 on when positive)
void __cdecl Xact_SetCueSpeakers(uint32_t cueHandle, int32_t speakers) {
    uint8_t* cue = static_cast<uint8_t*>(ValueOf(cues, cueHandle));
    if (cue != nullptr) {
        At<int32_t>(cue, 0x18) = speakers;
        if (speakers > 0) {
            At<uint8_t>(cue, 0x14) = 1;
        }
    }
}

// 0x5AD290: a cue's gains for a level
void __cdecl Xact_SetCueLevel(uint32_t cueHandle, int level, int) {
    void* cue = ValueOf(cues, cueHandle);
    if (cue != nullptr) {
        Voice_SetLevel(cue, level);
    }
}

// 0x5AD2D0
void __cdecl Xact_PanCueRotated(uint32_t cueHandle, int level, int angle, int) {
    void* cue = ValueOf(cues, cueHandle);
    if (cue != nullptr) {
        Speakers_PanRotated(cue, level, angle);
    }
}

// 0x5AD310
void __cdecl Xact_PanCueByAngle(uint32_t cueHandle, int level, int angle, float volume) {
    void* cue = ValueOf(cues, cueHandle);
    if (cue != nullptr) {
        Speakers_PanByAngle(cue, level, angle, volume);
    }
}

// 0x5AD370
void __cdecl Xact_PanCue51(uint32_t cueHandle, int level, int angle, float volume) {
    void* cue = ValueOf(cues, cueHandle);
    if (cue != nullptr) {
        Speakers_Pan51(cue, level, angle, volume);
    }
}

// 0x5AD3D0: panned by its speaker set (+0x18) as the angle
void __cdecl Xact_PanCue51Speakers(uint32_t cueHandle, int level, float volume) {
    uint8_t* cue = static_cast<uint8_t*>(ValueOf(cues, cueHandle));
    if (cue != nullptr) {
        Speakers_Pan51(cue, level, At<int32_t>(cue, 0x18), volume);
    }
}

// 0x5AD420
void __cdecl Xact_SilenceCue(uint32_t cueHandle) {
    void* cue = ValueOf(cues, cueHandle);
    if (cue != nullptr) {
        Voice_Silence(cue);
    }
}

// 0x5AD460
void __cdecl Xact_SetCueGains(uint32_t cueHandle, float a, float b, float c, float d, float e, float f) {
    void* cue = ValueOf(cues, cueHandle);
    if (cue != nullptr) {
        Voice_SetGains(cue, a, b, c, d, e, f);
    }
}

// ---- the shared data's updates ------------------------------------------------------------------

void MarkLoaded(PtrMap* map, uint32_t handle) {
    Lock(dataSection);
    MapItem* item = Find(map, handle);
    if (item != nullptr && item->value != nullptr) {
        At<uint8_t>(item->value, 4) = 1;
    }
    Unlock(dataSection);
}
// 0x5AD4D0, 0x5AD530, 0x5AD590: loaded
void __cdecl Data_MarkWaveBankImpl(uint32_t handle) { MarkLoaded(waveBankData, handle); }
void __cdecl Data_MarkSoundBankImpl(uint32_t handle) { MarkLoaded(soundBankData, handle); }
void __cdecl Data_MarkCueImpl(uint32_t handle) { MarkLoaded(cueData, handle); }

void RemoveData(PtrMap* map, uint32_t handle) {
    Lock(dataSection);
    MapItem* item = Find(map, handle);
    if (item != nullptr && item->value != nullptr) {
        void* value = item->value;
        Map_RemoveAt(map, handle);
        Free(value);
    }
    Unlock(dataSection);
}
// 0x5AD5F0, 0x5AD660, 0x5AD6D0: removed
void __cdecl Data_RemoveWaveBankImpl(uint32_t handle) { RemoveData(waveBankData, handle); }
void __cdecl Data_RemoveSoundBankImpl(uint32_t handle) { RemoveData(soundBankData, handle); }
void __cdecl Data_RemoveCueImpl(uint32_t handle) { RemoveData(cueData, handle); }

// 0x5AD740: a wave bank's state
void __cdecl Data_SetWaveBankStateImpl(uint32_t handle, uint32_t state) {
    Lock(dataSection);
    MapItem* item = Find(waveBankData, handle);
    if (item != nullptr && item->value != nullptr) {
        At<uint32_t>(item->value, 8) = state;
    }
    Unlock(dataSection);
}

// 0x5AD7A0: a cue's state and channels
void __cdecl Data_SetCueStateImpl(uint32_t handle, uint32_t state, uint32_t channels) {
    Lock(dataSection);
    MapItem* item = Find(cueData, handle);
    if (item != nullptr && item->value != nullptr) {
        At<uint32_t>(item->value, 8) = state;
        At<uint32_t>(item->value, 0xC) = channels;
    }
    Unlock(dataSection);
}

// ---- reference counts on the XACT objects ({object, count}) -------------------------------------

void AddRef(PtrMap* map, void* object) {
    if (object == nullptr) {
        return;
    }
    MapItem* item = Find(map, Key(object));
    if (item != nullptr) {
        if (item->value != nullptr) {
            At<int32_t>(item->value, 4) += 1;
        }
        return;
    }
    uint8_t* count = static_cast<uint8_t*>(New(8));
    if (count != nullptr) {
        At<void*>(count, 0) = nullptr;
        At<int32_t>(count, 4) = 0;
    }
    At<int32_t>(count, 4) = 1;   // (a failed allocation writes through null)
    At<void*>(count, 0) = object;
    Map_SetAt(map, Key(object), count);
}

// 0x5AD810, 0x5AD8E0, 0x5AD9B0: another reference
void __cdecl Ref_AddWaveBankImpl(void* bank) { AddRef(waveBankRefs, bank); }
void __cdecl Ref_AddSoundBankImpl(void* bank) { AddRef(soundBankRefs, bank); }
void __cdecl Ref_AddCue(void* cue) { AddRef(cueRefs, cue); }

// the last reference gone: the object's destroy method (vtable slot)
bool Release(PtrMap* map, void* object) {
    if (object == nullptr) {
        return false;
    }
    MapItem* item = Find(map, Key(object));
    if (item == nullptr || item->value == nullptr) {
        return false;
    }
    void* count = item->value;
    if (--At<int32_t>(count, 4) != 0) {
        return false;
    }
    Map_RemoveAt(map, Key(object));
    Free(count);
    return true;
}
using ComFn = long(__stdcall*)(void*);

// 0x5AD880: a wave bank (IXACTWaveBank::Destroy)
void __cdecl Ref_ReleaseWaveBank(void* bank) {
    if (Release(waveBankRefs, bank)) {
        re::PtrCast<ComFn>(re::ComVtable(bank)[0])(bank);
    }
}

// 0x5AD950: a sound bank (IXACTSoundBank::Destroy)
void __cdecl Ref_ReleaseSoundBankImpl(void* bank) {
    if (Release(soundBankRefs, bank)) {
        re::PtrCast<ComFn>(re::ComVtable(bank)[0x18 / 4])(bank);
    }
}

// 0x5ADA20: a cue (IXACTCue::Destroy; its notification ignored meanwhile)
void __cdecl Ref_ReleaseCueImpl(void* cue) {
    if (Release(cueRefs, cue)) {
        destroyingCue = cue;
        re::PtrCast<ComFn>(re::ComVtable(cue)[3])(cue);
        destroyingCue = nullptr;
    }
}

// 0x5ADA90: pauses every sound bank's flagged cues
void __cdecl Xact_PauseSoundBanks() {
    PtrMap* map = soundBanks;
    for (MapItem* item = FirstItem(map); item != nullptr;) {
        MapItem* next = NextItem(map, item);
        if (item->value != nullptr) {
            SoundBank_PauseCues(item->value);
            map = soundBanks;
        }
        item = next;
    }
}

// 0x5ADB20: resumes them
void __cdecl Xact_ResumeSoundBanks() {
    PtrMap* map = soundBanks;
    for (MapItem* item = FirstItem(map); item != nullptr;) {
        MapItem* next = NextItem(map, item);
        if (item->value != nullptr) {
            SoundBank_ResumeCues(item->value);
            map = soundBanks;
        }
        item = next;
    }
}

// 0x5ADBB0: the per-frame update: the queued commands, the sound banks' cues, XACT's work,
// then the wave banks' and cues' states into the shared data
void __cdecl Xact_Update(uint32_t now) {
    if (xactUpdating != 0) {
        return;
    }
    xactUpdating = 1;
    if (xactEngine != nullptr) {
        Lock(commandSection);
        uint8_t* command = pendingCommands;
        pendingCommands = nullptr;
        pendingCommandsTail = 0;
        Unlock(commandSection);
        using ExecuteFn = void(__thiscall*)(void*);
        using DeleteFn = void*(__thiscall*)(void*, int);
        while (command != nullptr) {
            re::PtrCast<ExecuteFn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(command))[1])(command);
            uint8_t* next = At<uint8_t*>(command, 4);
            re::PtrCast<DeleteFn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(command))[0])(command, 1);
            command = next;
        }
        re::Ptr32<void> position = Map_GetFirstPos(soundBanks);
        while (position != nullptr) {
            uint32_t key;
            re::Ptr32<void> bank = nullptr;
            Map_GetNext(soundBanks, &position, &key, &bank);
            if (bank != nullptr) {
                SoundBank_Update(bank, now);
            }
        }
        if (xactEngine != nullptr) {
            re::PtrCast<ComFn>(re::ComVtable(xactEngine)[0x20 / 4])(xactEngine);
        }
        position = Map_GetFirstPos(waveBanks);
        while (position != nullptr) {
            uint32_t key;
            re::Ptr32<void> bank = nullptr;
            Map_GetNext(waveBanks, &position, &key, &bank);
            if (bank != nullptr) {
                uint32_t state = 0;
                WaveBank_GetState(bank, &state);
                Data_SetWaveBankState(At<int32_t>(bank, 8), state);
            }
        }
        position = Map_GetFirstPos(cues);
        while (position != nullptr) {
            uint32_t key;
            re::Ptr32<void> cue = nullptr;
            Map_GetNext(cues, &position, &key, &cue);
            if (cue != nullptr) {
                uint32_t state = 0;
                Cue_GetState(cue, &state);
                uint8_t properties[3] = {0, 0, 0};
                uint32_t channels = 0;
                if (Cue_GetProperties(cue, properties) != 0) {
                    channels = properties[2];
                }
                Data_SetCueState(At<int32_t>(cue, 4), state, channels);
            }
        }
    }
    xactUpdating = 0;
}

// ---- start-up and the command handlers -------------------------------------------------------

// 0x5AE2A0: starts XACT with the global settings (copied), the nine maps, and the
// cue-destroyed notification
uint8_t __cdecl Xact_Init(const void* settings, uint32_t size) {
    using CoInitializeExFn = long(__stdcall*)(void*, uint32_t);
    if (re::GameFnAt<CoInitializeExFn>(uintptr_t(0x008FC328))(nullptr, 0) < 0) {
        return 0;
    }
    if (XactCreateEngine(0, &xactEngine) < 0 || xactEngine == nullptr) {
        return 0;
    }
    alignas(4) uint8_t details[0x400];
    At<uint16_t>(details, 0) = 0;
    Memset(details + 2, 0, 0x1FC);
    Memset(details + 0x1FE, 0, 0x202);
    using GetRendererDetailsFn = long(__stdcall*)(void*, uint32_t, void*);
    uint8_t* engine = xactEngine;
    if (re::PtrCast<GetRendererDetailsFn>(re::ComVtable(engine)[0x10 / 4])(engine, 0, details) < 0) {
        return 0;
    }
    xactSettings = New(size);
    Memcpy(xactSettings, settings, size);
    xactSettingsSize = size;
    alignas(4) uint8_t parameters[0x3C] = {};
    struct {
        re::Ptr32<PtrMap>* map;
        void* (__thiscall* construct)(void*, uint32_t);
        uint32_t size;
    } maps[] = {{&waveBankData, Map_Construct0, 0x47}, {&soundBankData, Map_Construct1, 0x47},
                {&cueData, Map_Construct2, 0xEF},      {&waveBanks, Map_Construct3, 0x47},
                {&soundBanks, Map_Construct4, 0x47},   {&cues, Map_Construct5, 0xEF},
                {&waveBankRefs, Map_Construct6, 0x47}, {&soundBankRefs, Map_Construct7, 0x47},
                {&cueRefs, Map_Construct8, 0xEF}};
    for (auto& map : maps) {
        void* memory = NiNew(0x10);
        *map.map = memory != nullptr ? static_cast<PtrMap*>(map.construct(memory, map.size)) : nullptr;
    }
    At<uint32_t>(parameters, 0) = 0xFA;                          // lookAheadTime
    At<void*>(parameters, 4) = xactSettings;                     // pGlobalSettingsBuffer
    At<uint32_t>(parameters, 8) = xactSettingsSize;
    At<uint32_t>(parameters, 0xC) = 0;
    At<uint32_t>(parameters, 0x10) = 0x99040000;
    At<uint32_t>(parameters, 0x1C) = 0x005ACD90;               // the notification callback
    At<void*>(parameters, 0x20) = details;                       // pRendererID
    using InitializeFn = long(__stdcall*)(void*, void*);
    engine = xactEngine;
    if (re::PtrCast<InitializeFn>(re::ComVtable(engine)[0x18 / 4])(engine, parameters) < 0) {
        return 0;
    }
    alignas(4) uint8_t notification[0x1A];
    std::memset(notification + 1, 0, 0x19);
    notification[1] = 1;       // flags
    notification[0] = 4;       // XACTNOTIFICATIONTYPE_CUEDESTROYED
    At<uint16_t>(notification, 0x12) = 0xFFFF;
    using RegisterFn = long(__stdcall*)(void*, void*);
    engine = xactEngine;
    re::PtrCast<RegisterFn>(re::ComVtable(engine)[0x3C / 4])(engine, notification);
    if (xactInitialised != nullptr && xactEngine != nullptr) {
        xactReverbType = 0;
    }
    xactReverbForced = 0;
    destroyingCue = nullptr;
    return 1;
}

// 0x5AE590: loads a sound bank from memory (the buffer freed on failure)
void __cdecl Xact_LoadSoundBank(int32_t handle, void* data, uint32_t size, int32_t, int32_t) {
    uint8_t* engine = xactEngine;
    if (engine == nullptr) {
        return;
    }
    void* xactBank = nullptr;
    using CreateSoundBankFn = long(__stdcall*)(void*, void*, uint32_t, uint32_t, uint32_t, void**);
    if (re::PtrCast<CreateSoundBankFn>(re::ComVtable(engine)[0x24 / 4])(engine, data, size, 0, 0,
                                                                                            &xactBank) < 0) {
        Free(data);
        Data_RemoveSoundBank(handle);
        return;
    }
    Ref_AddSoundBank(xactBank);
    Data_MarkSoundBank(handle);
    void* memory = New(0x18);
    uint8_t* bank = memory != nullptr ? static_cast<uint8_t*>(SoundBank_Construct(memory, xactBank, handle)) : nullptr;
    Map_SetAt(soundBanks, At<uint32_t>(bank, 0x14), bank);
}

// 0x5AE630: unloads a sound bank
void __cdecl Xact_UnloadSoundBank(uint32_t handle) {
    PtrMap* map = soundBanks;
    MapItem* item = Find(map, handle);
    if (item == nullptr || item->value == nullptr) {
        return;
    }
    void* bank = item->value;
    Map_RemoveAt(map, handle);
    SoundBank_Release(bank);
    SoundBank_Tidy(bank);
    Free(bank);
    Data_RemoveSoundBank(int32_t(handle));
}

// 0x5AE690: loads a wave bank from memory
void __cdecl Xact_LoadWaveBank(int32_t handle, void* data, uint32_t size, uint32_t flags, uint32_t allocation) {
    uint8_t* engine = xactEngine;
    if (engine == nullptr) {
        return;
    }
    void* xactBank = nullptr;
    using CreateWaveBankFn = long(__stdcall*)(void*, void*, uint32_t, uint32_t, uint32_t, void**);
    if (re::PtrCast<CreateWaveBankFn>(re::ComVtable(engine)[0x28 / 4])(engine, data, size, flags,
                                                                                           allocation, &xactBank) != 0) {
        Free(data);
        Data_RemoveWaveBank(handle);
        return;
    }
    Ref_AddWaveBank(xactBank);
    Data_MarkWaveBank(handle);
    void* memory = New(0xC);
    uint8_t* bank = memory != nullptr ? static_cast<uint8_t*>(WaveBank_Construct(memory, xactBank, 0, handle)) : nullptr;
    Map_SetAt(waveBanks, At<uint32_t>(bank, 8), bank);
}

// 0x5AE730: creates a streaming wave bank
void __cdecl Xact_LoadStreamingWaveBank(int32_t handle, void* parameters) {
    uint8_t* engine = xactEngine;
    if (engine == nullptr) {
        return;
    }
    void* xactBank = nullptr;
    using CreateStreamingFn = long(__stdcall*)(void*, void*, void**);
    if (re::PtrCast<CreateStreamingFn>(re::ComVtable(engine)[0x2C / 4])(engine, parameters,
                                                                                           &xactBank) != 0) {
        Data_RemoveWaveBank(handle);
        return;
    }
    Ref_AddWaveBank(xactBank);
    Data_MarkWaveBank(handle);
    void* memory = New(0xC);
    uint8_t* bank = memory != nullptr ? static_cast<uint8_t*>(WaveBank_Construct(memory, xactBank, 1, handle)) : nullptr;
    Map_SetAt(waveBanks, At<uint32_t>(bank, 8), bank);
}

// 0x5AE7B0: unloads a wave bank
void __cdecl Xact_UnloadWaveBank(uint32_t handle) {
    PtrMap* map = waveBanks;
    MapItem* item = Find(map, handle);
    if (item == nullptr || item->value == nullptr) {
        return;
    }
    void* bank = item->value;
    Map_RemoveAt(map, handle);
    WaveBank_Release(bank);
    EmptyThiscall(bank);
    Free(bank);
    Data_RemoveWaveBank(int32_t(handle));
}

// 0x5AE810: prepares a sound bank's cue by name (a NiFixedString made from it)
void __cdecl Xact_PrepareCueByName(uint32_t bankHandle, int32_t cueHandle, const char* name) {
    re::Ptr32<const char> fixed = name != nullptr ? NiFixedString_Create(name) : static_cast<const char*>(*emptyFixedString);
    MapItem* item = Find(soundBanks, bankHandle);
    if (item != nullptr && item->value != nullptr) {
        uint8_t* cue = SoundBank_PrepareCueByName(item->value, &fixed);
        if (cue != nullptr) {
            At<int32_t>(cue, 4) = cueHandle;
            Data_MarkCue(cueHandle);
            Map_SetAt(cues, At<uint32_t>(cue, 4), cue);
            NiFixedString_Destruct(&fixed);
            return;
        }
    }
    Data_RemoveCue(cueHandle);
    if (fixed != *emptyFixedString) {
        uint32_t length = *reinterpret_cast<const uint16_t*>(fixed - 2);
        using InterlockedFn = long(__stdcall*)(volatile long*);
        long left = re::GameFnAt<InterlockedFn>(uintptr_t(0x008FC170))(
            reinterpret_cast<volatile long*>(const_cast<char*>(fixed - 8)));
        if (left == 1) {
            NiStringTable_Free(*reinterpret_cast<re::Ptr32<void>*>(0x020E216C), &fixed, length);
        }
    }
}

// 0x5AE8E0: prepares a sound bank's cue by index
void __cdecl Xact_PrepareCue(uint32_t bankHandle, int32_t cueHandle, int32_t index) {
    MapItem* item = Find(soundBanks, bankHandle);
    if (item == nullptr || item->value == nullptr) {
        Data_RemoveCue(cueHandle);
        return;
    }
    void* bank = item->value;
    re::Ptr32<const char> name;
    SoundBank_GetCueName(bank, &name, index);
    uint8_t* cue = SoundBank_PrepareCue(bank, &name, index);
    if (cue != nullptr) {
        At<int32_t>(cue, 4) = cueHandle;
        Data_MarkCue(cueHandle);
        Map_SetAt(cues, At<uint32_t>(cue, 4), cue);
        NiFixedString_Destruct(&name);
        return;
    }
    NiFixedString_Destruct(&name);
    Data_RemoveCue(cueHandle);
}

// 0x5AE990: forgets a cue (its map entry and data)
void __cdecl Cues_RemoveImpl(uint8_t* cue) {
    if (cue == nullptr) {
        return;
    }
    Map_RemoveAt(cues, At<uint32_t>(cue, 4));
    Data_RemoveCue(At<int32_t>(cue, 4));
}

// ---- registration ---------------------------------------------------------------------------

#define MDXMGR_REVERSED(name, address, fn)                                                       \
    static void* RE_CONCAT(mdxmgr_original_, __LINE__);                                          \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(mdxmgr_original_, __LINE__), \
             re::Mode::Original)

MDXMGR_REVERSED("MDXactData::WaveBankState", 0x005AC870, &Data_WaveBankState);
MDXMGR_REVERSED("MDXactData::SoundBankState", 0x005AC8E0, &Data_SoundBankState);
MDXMGR_REVERSED("MDXactData::CueState", 0x005AC950, &Data_CueState);
MDXMGR_REVERSED("MDXactData::AddWaveBank", 0x005AC9C0, &Data_AddWaveBankImpl);
MDXMGR_REVERSED("MDXactData::AddSoundBank", 0x005ACA10, &Data_AddSoundBankImpl);
MDXMGR_REVERSED("MDXactData::AddCue", 0x005ACA50, &Data_AddCueImpl);
MDXMGR_REVERSED("MDXactData::SetCuePaused", 0x005ACAA0, &Data_SetCuePaused);
MDXMGR_REVERSED("MDXactData::SetCueSpeakers", 0x005ACB00, &Data_SetCueSpeakers);
MDXMGR_REVERSED("MDXactData::GetWaveBankState", 0x005ACB60, &Data_GetWaveBankState);
MDXMGR_REVERSED("MDXactData::GetCuePaused", 0x005ACBD0, &Data_GetCuePaused);
MDXMGR_REVERSED("MDXactData::GetCueSpeakers", 0x005ACC40, &Data_GetCueSpeakers);
MDXMGR_REVERSED("MDXactData::GetCueState", 0x005ACCB0, &Data_GetCueState);
MDXMGR_REVERSED("MDXactData::GetCueChannels", 0x005ACD20, &Data_GetCueChannels);
MDXMGR_REVERSED("MDXact::OnNotification", 0x005ACD90, &Xact_OnNotification);
MDXMGR_REVERSED("MDXact::StopCueOf", 0x005ACF70, &Xact_StopCueOf);
MDXMGR_REVERSED("MDXact::DestroyCueOf", 0x005ACFE0, &Xact_DestroyCueOf);
MDXMGR_REVERSED("MDXact::StopCueIfDone", 0x005AD050, &Xact_StopCueIfDone);
MDXMGR_REVERSED("MDXact::PlayCue", 0x005AD0C0, &Xact_PlayCue);
MDXMGR_REVERSED("MDXact::PauseCue", 0x005AD110, &Xact_PauseCue);
MDXMGR_REVERSED("MDXact::SetCueVariableB", 0x005AD150, &Xact_SetCueVariableB);
MDXMGR_REVERSED("MDXact::SetCueVariableA", 0x005AD1A0, &Xact_SetCueVariableA);
MDXMGR_REVERSED("MDXact::SetCueReverbMix", 0x005AD1F0, &Xact_SetCueReverbMix);
MDXMGR_REVERSED("MDXact::SetCueSpeakers", 0x005AD240, &Xact_SetCueSpeakers);
MDXMGR_REVERSED("MDXact::SetCueLevel", 0x005AD290, &Xact_SetCueLevel);
MDXMGR_REVERSED("MDXact::PanCueRotated", 0x005AD2D0, &Xact_PanCueRotated);
MDXMGR_REVERSED("MDXact::PanCueByAngle", 0x005AD310, &Xact_PanCueByAngle);
MDXMGR_REVERSED("MDXact::PanCue51", 0x005AD370, &Xact_PanCue51);
MDXMGR_REVERSED("MDXact::PanCue51Speakers", 0x005AD3D0, &Xact_PanCue51Speakers);
MDXMGR_REVERSED("MDXact::SilenceCue", 0x005AD420, &Xact_SilenceCue);
MDXMGR_REVERSED("MDXact::SetCueGains", 0x005AD460, &Xact_SetCueGains);
MDXMGR_REVERSED("MDXactData::MarkWaveBank", 0x005AD4D0, &Data_MarkWaveBankImpl);
MDXMGR_REVERSED("MDXactData::MarkSoundBank", 0x005AD530, &Data_MarkSoundBankImpl);
MDXMGR_REVERSED("MDXactData::MarkCue", 0x005AD590, &Data_MarkCueImpl);
MDXMGR_REVERSED("MDXactData::RemoveWaveBank", 0x005AD5F0, &Data_RemoveWaveBankImpl);
MDXMGR_REVERSED("MDXactData::RemoveSoundBank", 0x005AD660, &Data_RemoveSoundBankImpl);
MDXMGR_REVERSED("MDXactData::RemoveCue", 0x005AD6D0, &Data_RemoveCueImpl);
MDXMGR_REVERSED("MDXactData::SetWaveBankState", 0x005AD740, &Data_SetWaveBankStateImpl);
MDXMGR_REVERSED("MDXactData::SetCueState", 0x005AD7A0, &Data_SetCueStateImpl);
MDXMGR_REVERSED("MDXactRefs::AddWaveBank", 0x005AD810, &Ref_AddWaveBankImpl);
MDXMGR_REVERSED("MDXactRefs::ReleaseWaveBank", 0x005AD880, &Ref_ReleaseWaveBank);
MDXMGR_REVERSED("MDXactRefs::AddSoundBank", 0x005AD8E0, &Ref_AddSoundBankImpl);
MDXMGR_REVERSED("MDXactRefs::ReleaseSoundBank", 0x005AD950, &Ref_ReleaseSoundBankImpl);
MDXMGR_REVERSED("MDXactRefs::AddCue", 0x005AD9B0, &Ref_AddCue);
MDXMGR_REVERSED("MDXactRefs::ReleaseCue", 0x005ADA20, &Ref_ReleaseCueImpl);
MDXMGR_REVERSED("MDXact::PauseSoundBanks", 0x005ADA90, &Xact_PauseSoundBanks);
MDXMGR_REVERSED("MDXact::ResumeSoundBanks", 0x005ADB20, &Xact_ResumeSoundBanks);
MDXMGR_REVERSED("MDXact::Update", 0x005ADBB0, &Xact_Update);
MDXMGR_REVERSED("MDXact::Init", 0x005AE2A0, &Xact_Init);
MDXMGR_REVERSED("MDXact::LoadSoundBank", 0x005AE590, &Xact_LoadSoundBank);
MDXMGR_REVERSED("MDXact::UnloadSoundBank", 0x005AE630, &Xact_UnloadSoundBank);
MDXMGR_REVERSED("MDXact::LoadWaveBank", 0x005AE690, &Xact_LoadWaveBank);
MDXMGR_REVERSED("MDXact::LoadStreamingWaveBank", 0x005AE730, &Xact_LoadStreamingWaveBank);
MDXMGR_REVERSED("MDXact::UnloadWaveBank", 0x005AE7B0, &Xact_UnloadWaveBank);
MDXMGR_REVERSED("MDXact::PrepareCueByName", 0x005AE810, &Xact_PrepareCueByName);
MDXMGR_REVERSED("MDXact::PrepareCue", 0x005AE8E0, &Xact_PrepareCue);
MDXMGR_REVERSED("MDXact::RemoveCue", 0x005AE990, &Cues_RemoveImpl);

} // namespace
