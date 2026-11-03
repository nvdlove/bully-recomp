// The speech system (0x59FC20..0x5A2F90): the speech library (0xCF4E10, vtable 0x9211A8:
// 16 queue items of 0x48 bytes, the event packages by id at +0x4D0) and the speech
// manager (0xCF4378: two playing slots of 0x5C bytes at +0x214, the current one's index
// at +0x2CC, a pool of 16 pending records of 0x14 bytes at +0xD0, the per-event filter
// callbacks at +0x2D0, the last result at +0xA88); the speech events' filters (cdecl
// (speaker, target, event*)), which pick the line and its priority.
//
// A speech event (0x30 bytes, passed around by pointer and once by value):
//   +0   bits 0..8 the event id (0x1FF none), 9..16 the speaker's model, 17..24 the priority
//   +4   bits 0..10 the variant
//   +8   flags (1 queued, 8 check priority, 0x10 check the slots, 0x88 and +9 & 5 urgent)
//   +0xC bits 0..1 a counter the filters bump
//   +0x10 the timeout in ms

#include <cstdint>
#include <cstring>

#include "reversed.h"

#include "game/Crt.h"
namespace {

struct CVector {
    float x, y, z;
};

using re::At;

struct SpeechEvent {
    uint32_t word0;      // +0 id / model / priority
    uint32_t word4;      // +4 variant
    uint32_t flags8;     // +8
    uint32_t wordC;      // +0xC
    uint32_t timeout;    // +0x10
    uint32_t unknown14[7];
};
static_assert(sizeof(SpeechEvent) == 0x30);

// a playing slot (0x5C bytes, two at 0xCF458C)
struct SpeechSlot {
    int32_t state;       // +0 (0 free, 4 finishing, 3 ...)
    uint8_t unknown4[0xC];
    re::Ptr32<void> ped;           // +0x10 (a registered reference)
    uint32_t unknown14;
    SpeechEvent event;   // +0x18
    uint8_t unknown48[0x14];
};
static_assert(sizeof(SpeechSlot) == 0x5C);

inline uint8_t* const TheSpeech = reinterpret_cast<uint8_t*>(0x00CF4378);
inline uint8_t* const TheSpeechLib = reinterpret_cast<uint8_t*>(0x00CF4E10);
GAME_ARRAY(SpeechSlot, speechSlots, 0x00CF458C);        // TheSpeech + 0x214
GAME_VAR(int32_t, speechCurrentSlot, 0x00CF4644);      // TheSpeech + 0x2CC
GAME_ARRAY(void*, streamChannelList, 0x00CF4290);       // one per slot
GAME_VAR(float, speechListenerX, 0x00CF4438);
GAME_VAR(float, speechListenerY, 0x00CF443C);
GAME_VAR(float, speechListenerZ, 0x00CF4440);
GAME_VAR(float, speechLoudScale, 0x00CF442C);          // (percent)
GAME_VAR(uint8_t, missionsResolved, 0x00AE80B8);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t, speechBlockedA, 0x020C5BE6);
GAME_VAR(uint8_t, soundsPaused, 0x00CF3DA2);
GAME_VAR(uint8_t, streamsBlocked, 0x00CF4286);
GAME_VAR(uint8_t, speechDebugNames, 0x00CF3DD9);
GAME_VAR(uint8_t, speechAlways, 0x00CF3DA3);
GAME_VAR(void*, nearPlane, 0x00C3CCF4);
// the player's clothes (bytes, 0xFF none) and the outfit table [clothing * 11 + slot]
GAME_VAR(int8_t, clothesSlot0, 0x020C5F79);
GAME_VAR(int8_t, clothesSlot1, 0x020C5F85);
GAME_VAR(int8_t, clothesSlot2, 0x020C5FA9);
GAME_VAR(int8_t, clothesSlot3, 0x020C5FB5);
GAME_ARRAY(int32_t, clothingKinds, 0x020C78FC);
GAME_VAR(int32_t, outfitCurrent, 0x020C5F70);
GAME_VAR(int32_t, outfitUniform, 0x020C5F6C);
GAME_VAR(int32_t, playerDirt, 0x020C78E0);
GAME_VAR(uint8_t, playerFlags5F87, 0x020C5F87);
inline void* const TheMissions = reinterpret_cast<void*>(0x020C3CA0);
inline void* const TheText = reinterpret_cast<void*>(0x020C2FA4);
inline const void* const missingText = reinterpret_cast<const void*>(0x0093F0E8);
inline void* const subtitles = reinterpret_cast<void*>(0x0208D388);
inline void* const speechHistory = reinterpret_cast<void*>(0x0208B9F0);

inline void* __cdecl Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(int, __cdecl, RandomInt, 0x0040DA80, int low, int high);
template <typename... A>
inline int __cdecl Sprintf(char* buffer, const char* format, A... rest) { return crt::Sprintf(buffer, format, rest...); }
inline int __cdecl AtoI(const char* s) { return (int)crt::Atol(s); }
GAME_FN(uint8_t*, __cdecl, GetStreamingInfo, 0x0052DD30, int id);
GAME_FN(void*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, uint32_t* self);
GAME_FN(int32_t, __thiscall, Missions_Find, 0x006A9FD0, void* missions, uint32_t* key);
GAME_FN(uint8_t*, __thiscall, Missions_GetStats, 0x006AA7C0, void* missions, int32_t mission);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, void* ref, void* entity);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, void* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(int32_t*, __thiscall, Ped_GetAttitude, 0x00475400, void* ped, int32_t* out, void* other);
GAME_FN(uint8_t, __thiscall, PedFlags_Fn48EBA0, 0x0048EBA0, void* flags);
GAME_FN(const char*, __cdecl, PedTypeName, 0x00488AF0, int pedType);
GAME_FN(uint8_t, __cdecl, IsEntityPointerValid, 0x004657F0, void* entity);
GAME_FN(void*, __cdecl, GetCameraNode, 0x006C82F0, void* camera);
GAME_FN(uint8_t*, __cdecl, GetLocalTransform, 0x006C9A90, void* node);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414BA0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(uint8_t, __cdecl, PlayerWearing_601410, 0x00601410, void* ped, int kind, int);
GAME_FN(uint8_t, __cdecl, SpeechModelMatches, 0x0059E8E0, int model, int other);
GAME_FN(int, __cdecl, SFX_Fn59E3E0, 0x0059E3E0, float distance, int);
GAME_FN(int32_t, __thiscall, Channel_IsIdle, 0x0058E060, void* channel);
GAME_FN(int32_t, __thiscall, Channel_Fn5A6D60, 0x005A6D60, void* channel);
GAME_FN(int32_t, __thiscall, Channel_Fn5A6D80, 0x005A6D80, void* channel);
GAME_FN(int32_t, __thiscall, Channel_Fn5A6DA0, 0x005A6DA0, void* channel);
GAME_FN(uint8_t, __thiscall, Channel_Preload, 0x005A7880, void* channel, const char* name, uint32_t hash,
        float volume, int ms, int, int useSpeech);
GAME_FN(const void*, __thiscall, Text_Get, 0x0068FE80, void* text, const char* key);
GAME_FN(void, __thiscall, Subtitles_Add, 0x0068F160, void* subtitles, const char* key, int, int);
GAME_FN(int32_t, __thiscall, SpeechHistory_Add, 0x0068E190, void* history, int32_t slot, int32_t id, int flag);
// the speech library's
GAME_FN(void, __thiscall, SpeechLib_Remove, 0x0059F310, void* lib, void* item);
GAME_FN(void, __thiscall, SpeechLib_Reset, 0x0059F290, void* lib);
GAME_FN(int32_t, __thiscall, SpeechLib_Pick, 0x0059F620, void* lib, void* package, SpeechEvent* ev, re::Ptr32<void>* out);
GAME_FN(uint8_t, __thiscall, SpeechLib_Add, 0x0059F800, void* lib, SpeechEvent* ev, void* item, void* arg);
GAME_FN(void, __thiscall, SpeechLib_ForgetModel, 0x0059F110, void* lib, int model);
GAME_FN(uint8_t, __thiscall, SpeechLib_Has, 0x0059F3E0, void* lib, int id, int model);
GAME_FN(uint8_t, __thiscall, SpeechLib_IsSaying, 0x0059F1A0, void* lib, void* ped, int id, int variant);
GAME_FN(int32_t, __thiscall, SpeechEvent_Compare, 0x0059E7B0, void* speech, SpeechEvent* a, SpeechEvent* b);
// this file's
GAME_FN(uint8_t*, __thiscall, SpeechItems_Init, 0x0059FC20, void* items);
GAME_FN(int32_t, __thiscall, SpeechLib_Queue, 0x0059FCC0, void* lib, SpeechEvent* ev, void* arg);
GAME_FN(uint8_t, __thiscall, SpeechMission_IsDone, 0x0059FEE0, void* self);
GAME_FN(int, __cdecl, Speech_IsLoud, 0x005A0530, SpeechEvent ev);
GAME_FN(uint8_t*, __thiscall, Speech_AllocRecord, 0x005A07B0, void* speech);
GAME_FN(uint8_t, __stdcall, Speech_IsAllowedForPlayer, 0x005A0810, int id);
GAME_FN(uint8_t, __thiscall, Speech_IsPedSaying, 0x005A0850, void* speech, void* ped, uint32_t priority);
GAME_FN(uint8_t, __thiscall, Speech_IsEnabled, 0x005A09B0, void* speech, SpeechEvent* ev);
GAME_FN(uint8_t, __cdecl, Speech_AnyChannelBusy, 0x005A0D20);
GAME_FN(int, __cdecl, Speech_PickClothingLine, 0x005A1620, void* ped, int reply);
GAME_FN(void, __thiscall, Speech_SetPriority, 0x005A1BA0, void* speech, void* ped, void* target, SpeechEvent* ev);
GAME_FN(uint8_t, __thiscall, Speech_CanInterrupt, 0x005A1DE0, void* speech, SpeechSlot* slot, SpeechEvent* ev);
GAME_FN(int32_t, __thiscall, Speech_CanStart, 0x005A1E80, void* speech, SpeechEvent* ev);

// a pointer to a ped (type 3) or null
uint8_t* AsPed(uint8_t* entity) {
    if (entity == nullptr || (At<uint8_t>(entity, 0x108) & 7) != 3) {
        return nullptr;
    }
    return entity;
}

// the event word's fields
constexpr uint32_t kIdMask = 0x1FF;
inline uint32_t SetId(uint32_t word, uint32_t id) {
    return (word & ~kIdMask) | id;
}
inline uint32_t SetPriority(uint32_t word, uint32_t priority) {
    return (word & 0xFE01FFFF) | (priority << 17);
}
inline uint32_t PriorityOf(uint32_t word) {
    return (word >> 17) & 0xFF;
}
inline void BumpCounter(uint32_t& word) {
    word = (((word + 1) ^ word) & 3) ^ word;
}
inline int16_t ModelOf(uint8_t* ped) {
    return At<int16_t>(ped, 0x10E);
}
// the stream channel of a slot (the game divides the slot's offset by 0x5C)
void* ChannelOfSlot(const SpeechSlot* slot) {
    int32_t offset = int32_t(reinterpret_cast<uintptr_t>(slot) - 0x00CF458C);
    return streamChannelList[offset / 0x5C];
}

// ---- the speech library ----------------------------------------------------------------------

// 0x59FC20 (thiscall): the 16 queue items (0x48 bytes from +4)
uint8_t* __fastcall SpeechItems_InitImpl(uint8_t* self) {
    uint8_t* item = self + 4;
    for (int i = 15; i >= 0; --i) {
        At<float>(item, 0x20) = 0.0f;
        At<float>(item, 0x24) = 0.0f;
        At<float>(item, 0x28) = 0.0f;
        Memset(item, 0, 0x30);
        At<uint32_t>(item, 0xC) &= ~3u;
        At<uint8_t>(item, 0x1C) &= 0xFE;
        At<int32_t>(item, 0x10) = 1000;
        At<uint8_t>(item, 8) = uint8_t((At<uint8_t>(item, 8) & 5) | 0xAA);
        At<uint8_t>(item, 9) = 0xF8;
        At<uint8_t>(item, 0xA) = uint8_t((At<uint8_t>(item, 0xA) & 0xFA) | 2);
        At<int32_t>(item, 0x2C) = -1;
        item += 0x48;
    }
    return self;
}

// 0x59FC90 (thiscall): disables it and removes every queued item
void __fastcall SpeechLib_Shutdown(uint8_t* self) {
    At<uint8_t>(self, 0x4CD) = 0;
    while (At<void*>(self, 0x48C) != nullptr) {
        SpeechLib_Remove(self, At<void*>(self, 0x48C));
    }
    SpeechLib_Reset(self);
}

// 0x59FCC0 (thiscall): queues an event: 0 done, -3 not added, -4 no package, -10 disabled
int32_t __fastcall SpeechLib_QueueImpl(uint8_t* self, re::Edx, SpeechEvent* ev, void* arg) {
    if (At<uint8_t>(self, 0x4CD) == 0) {
        return -10;
    }
    void* item;
    uint32_t id = ev->word0 & kIdMask;
    if (id != kIdMask) {
        uint16_t* package = At<uint16_t*>(self, 0x4D0 + int(id) * 4);
        if (package == nullptr || *package != id) {
            return -4;
        }
        re::Ptr32<void> picked;
        int32_t result = SpeechLib_Pick(self, package, ev, &picked);
        if (result != 0) {
            return result;
        }
        item = picked;
    } else {
        item = nullptr;
    }
    return SpeechLib_Add(self, ev, item, arg) != 0 ? 0 : -3;
}

// 0x59FD50 (thiscall): the constructor
uint8_t* __fastcall SpeechLib_Construct(uint8_t* self, re::Edx, int flag) {
    At<uint32_t>(self, 0) = 0x009211A8;
    SpeechItems_Init(self + 4);
    At<int32_t>(self, 0xC84) = 0;
    At<int32_t>(self, 0xC8C) = 0;
    At<int32_t>(self, 0x4A0) = 0;
    At<int32_t>(self, 0x4A4) = 0;
    At<int32_t>(self, 0x498) = 0;
    At<int32_t>(self, 0x4A8) = 0;
    At<int32_t>(self, 0x4AC) = 0;
    At<int32_t>(self, 0x4B0) = 0;
    At<int32_t>(self, 0x4B4) = 0;
    At<int32_t>(self, 0x4B8) = 0;
    At<int32_t>(self, 0x4C0) = 0;
    At<int32_t>(self, 0x4BC) = 0;
    At<int32_t>(self, 0x4C4) = 0;
    At<uint8_t>(self, 0x49C) = uint8_t(flag);
    At<int32_t>(self, 0x4C8) = 5;
    SpeechLib_Reset(self);
    At<uint8_t>(self, 0x4CD) = 1;
    std::memset(self + 0x4D0, 0, 0x1ED * 4);
    At<int32_t>(self, 0xC90) = 0;
    return self;
}

// 0x59FE00 (thiscall, an event): not urgent
int32_t __fastcall SpeechEvent_IsCalm(uint8_t* self) {
    if ((At<uint8_t>(self, 8) & 0x88) == 0 && (At<uint8_t>(self, 9) & 5) == 0) {
        return 1;
    }
    return 0;
}

// 0x59FE20: the speech stream (streaming id + 0x5B54) is loaded
uint8_t __cdecl Speech_IsStreamLoaded(int index) {
    return GetStreamingInfo(index + 0x5B54)[4] == 2 ? 1 : 0;
}

// 0x59FE40: one of the models with their own speech
uint8_t __cdecl Speech_IsSpecialModel(int32_t model) {
    static const uintptr_t models[] = {0x00BF4424, 0x00BF4420, 0x00BF4410, 0x00BF4408, 0x00BF43AC, 0x00BF438C,
                                       0x00BF4388, 0x00BF4404, 0x00BF43D4, 0x00BF43D0, 0x00BF43CC, 0x00BF4418};
    for (uintptr_t address : models) {
        if (model == *reinterpret_cast<int32_t*>(address)) {
            return 1;
        }
    }
    return 0;
}

// 0x59FEB0: the player holds one of two weapons (the model at his weapon slot +0x10E)
uint8_t __cdecl Speech_PlayerHasWeaponLine() {
    int32_t model = At<int16_t>(At<uint8_t*>(playerPed, 0x1D0), 0x10E);
    if (model == *reinterpret_cast<int32_t*>(0x00BF4430) || model == *reinterpret_cast<int32_t*>(0x00BF433C)) {
        return 1;
    }
    return 0;
}

// the mission table's index of a name
int32_t FindMission(const char* name) {
    uint32_t key;
    NameKey_Construct(&key, name);
    int32_t mission = Missions_Find(TheMissions, &key);
    NameKey_Destruct(&key);
    return mission;
}

// 0x59FEE0 (thiscall): mission 1_01's stats say it's played (both calls read +8: the
// second was meant to be 4_S12's at +0xC)
uint8_t __fastcall SpeechMission_IsDoneImpl(uint8_t* self) {
    if (missionsResolved != 0) {
        At<int32_t>(self, 8) = FindMission(reinterpret_cast<const char*>(0x00921884));
        At<int32_t>(self, 0xC) = FindMission(reinterpret_cast<const char*>(0x0092187C));   // "4_S12"
        missionsResolved = 0;
    }
    bool first = At<int16_t>(Missions_GetStats(TheMissions, At<int32_t>(self, 8)), 2) != 0;
    uint8_t* second = Missions_GetStats(TheMissions, At<int32_t>(self, 8));
    if (first && At<int16_t>(second, 2) != 0) {
        return 1;
    }
    return 0;
}

// 0x59FF80 (thiscall): the mission isn't one of the 37 story ones (the search runs over
// 40 entries: the last three are what follows the table)
uint8_t __fastcall SpeechMission_IsFree(uint8_t* self, re::Edx, int32_t mission) {
    if (At<uint8_t>(self, 4) == 0) {
        static const uintptr_t names[] = {
            0x902CC8, 0x9219A4, 0x92199C, 0x91D388, 0x921998, 0x92198C, 0x921980, 0x91D3A0, 0x91D3A8, 0x921978,
            0x921970, 0x91D024, 0x921964, 0x921958, 0x92187C, 0x921950, 0x921948, 0x921940, 0x921938, 0x921930,
            0x921928, 0x921920, 0x921918, 0x921910, 0x921908, 0x9218FC, 0x9218F4, 0x9218E8, 0x9218DC, 0x904F3C,
            0x904F24, 0x91D358, 0x9218CC, 0x9218BC, 0x9218AC, 0x92189C, 0x92188C};
        // 1_01 1_02 1_02A 1_10 1_B Chapt1Trans Chapt2Trans 2_09 2_B 3_06 3_08 3_B Chapt3Trans
        // Chapt4Trans 4_S12 4_02 4_06 4_B1 4_B2 5_09 5_06 5_07A 6_01 6_02 6_03 6_PassAll PriOff
        // 3_08_Launch 6_01_Launch Coaster Squid FerrisWheel C_Dodgeball_1..5
        for (int i = 0; i < 37; ++i) {
            At<int32_t>(self, 0x10 + i * 4) = FindMission(reinterpret_cast<const char*>(names[i]));
        }
        At<uint8_t>(self, 4) = 1;
    }
    for (int i = 0; i < 0x28; ++i) {
        if (At<int32_t>(self, 0x10 + i * 4) == mission) {
            return 0;
        }
    }
    return 1;
}

// ---- the speech manager -----------------------------------------------------------------------

// 0x5A0530: an event heard louder (+0xA & 4, or ids 0xEA, 0xFC, 0xFE); by value
int __cdecl Speech_IsLoudImpl(SpeechEvent ev) {
    if (((ev.flags8 >> 16) & 4) != 0) {
        return 1;
    }
    uint32_t id = ev.word0 & kIdMask;
    if (id == 0xFE || id == 0xEA || id == 0xFC) {
        return 1;
    }
    return 0;
}

// 0x5A0570 (thiscall): drops its references
void __fastcall Speech_ClearRefs(uint8_t* self) {
    SetRef(self + 0xA90, nullptr);
    for (int i = 1; i >= 0; --i) {
        SetRef(self + 0x214 + i * 0x5C + 0x10, nullptr);
    }
    for (int i = 15; i >= 0; --i) {
        SetRef(self + 0xD0 + i * 0x14 + 0xC, nullptr);
    }
}

// 0x5A0780: compares (a - b) with (c - d): -1, 0, 1
int32_t __stdcall CompareDifferences(int32_t a, int32_t b, int32_t c, int32_t d) {
    int32_t left = a - b;
    int32_t right = c - d;
    if (left < right) {
        return -1;
    }
    return left > right ? 1 : 0;
}

// 0x5A07B0 (thiscall): the next free record of the 16 (round robin), or null
uint8_t* __fastcall Speech_AllocRecordImpl(uint8_t* self) {
    for (int i = 0; i < 16; ++i) {
        int8_t index = At<int8_t>(self, 0x210);
        uint8_t* record = self + 0xD0 + index * 0x14;
        int8_t next = int8_t(index + 1);
        At<int8_t>(self, 0x210) = next;
        if (next >= 0x10) {
            At<int8_t>(self, 0x210) = 0;
        }
        if ((At<uint8_t>(record, 8) & 1) == 0) {
            At<uint8_t>(record, 8) |= 1;
            return record;
        }
    }
    return nullptr;
}

// 0x5A0810: false for the events 0x159..0x15E except 0x15C
uint8_t __stdcall Speech_IsAllowedForPlayerImpl(int id) {
    uint32_t index = uint32_t(id - 0x159);
    if (index > 5) {
        return 1;
    }
    return index == 3 ? 1 : 0;
}

bool SlotBusy(const SpeechSlot& slot) {
    return slot.state != 0 && slot.state != 4;
}

// 0x5A0850 (thiscall, uses the global slots): the ped (any if null) is speaking below the
// priority (-1 any); the second slot is compared with the first slot's priority
uint8_t __fastcall Speech_IsPedSayingImpl(void*, re::Edx, void* ped, uint32_t priority) {
    int32_t current = speechCurrentSlot;
    SpeechSlot& first = speechSlots[current];
    if (SlotBusy(first) && (ped == nullptr || first.ped == ped)) {
        if (priority == 0xFFFFFFFF || priority > PriorityOf(first.event.word0)) {
            return 1;
        }
    }
    SpeechSlot& second = speechSlots[(current - 1) & 1];
    if (SlotBusy(second) && (ped == nullptr || second.ped == ped)) {
        if (priority == 0xFFFFFFFF || priority > PriorityOf(first.event.word0)) {
            return 1;
        }
    }
    return 0;
}

// 0x5A08F0 (thiscall): the ped is saying the event (variant -1 any)
uint8_t __fastcall Speech_IsPedSayingEvent(void*, re::Edx, void* ped, uint32_t id, uint32_t variant) {
    int32_t current = speechCurrentSlot;
    SpeechSlot* slots[2] = {&speechSlots[current], &speechSlots[(current - 1) & 1]};
    for (SpeechSlot* slot : slots) {
        if (SlotBusy(*slot) && slot->ped == ped && (slot->event.word0 & kIdMask) == id) {
            if (variant == 0xFFFFFFFF || (slot->event.word4 & 0x7FF) == variant) {
                return 1;
            }
        }
    }
    return 0;
}

// 0x5A09A0 (thiscall): the filter flags' default
void __fastcall Speech_ResetFilter(uint8_t* self) {
    At<uint32_t>(self, 0xA8C) = 0xFFFFFF2F;
}

// 0x5A09B0 (thiscall): speech may play (the filter at +0xA8C: bit 7 on limits it to three
// models, +0xA8D..F, and a priority mask in the low bits)
uint8_t __fastcall Speech_IsEnabledImpl(uint8_t* self, re::Edx, SpeechEvent* ev) {
    uint32_t word = ev->word0;
    uint8_t priorityBit = uint8_t(1u << ((word >> 17) & 31));
    if (speechBlockedA != 0 || soundsPaused != 0 || streamsBlocked != 0) {
        return 0;
    }
    uint32_t filter = At<uint32_t>(self, 0xA8C);
    if (((filter >> 7) & 1) == 0) {
        return 1;
    }
    uint32_t model = (word >> 9) & 0xFF;
    if (At<uint8_t>(self, 0xA8D) == model || At<uint8_t>(self, 0xA8E) == model || At<uint8_t>(self, 0xA8F) == model) {
        return ((uint8_t(filter) & priorityBit) & 0x7F) != 0 ? 1 : 0;
    }
    return 0;
}

// 0x5A0A70 (thiscall): the number in a name's text (+8), counting the calls
int __fastcall Speech_ParseNumber(uint8_t* self, re::Edx, uint8_t* name) {
    char buffer[0x18];
    Sprintf(buffer, reinterpret_cast<const char*>(0x009219AC), name + 8);   // "%3s"
    At<int32_t>(self, 0xA84) += 1;
    return AtoI(buffer);
}

// 0x5A0AB0 (thiscall): a ped goes away: the slots forget him
void __fastcall Speech_ForgetPed(uint8_t* self, re::Edx, uint8_t* ped) {
    if (At<int32_t>(self, 0x214) != 0) {
        void* speaker = At<void*>(self, 0x224);
        if (speaker != nullptr && speaker == ped) {
            At<int32_t>(self, 0x240) = 0;
        }
    }
    if (At<int32_t>(self, 0x270) != 0) {
        void* speaker = At<void*>(self, 0x280);
        if (speaker != nullptr && speaker == ped) {
            At<int32_t>(self, 0x29C) = 0;
        }
    }
    SpeechLib_ForgetModel(TheSpeechLib, ModelOf(ped));
}

// 0x5A0B10 (thiscall): records the event in the history with the slot playing it
int32_t __fastcall Speech_RecordHistory(uint8_t* self, re::Edx, int32_t id) {
    uint8_t both = 0;
    int32_t slot = -1;
    int32_t current = At<int32_t>(self, 0x2CC);
    if (At<uint8_t>(self, 0xB1) != 0) {
        SpeechSlot* first = reinterpret_cast<SpeechSlot*>(self + 0x214) + current;
        if (first->state != 0 && int32_t(first->event.word0 & kIdMask) == id) {
            slot = current;
        }
        int32_t other = (current - 1) & 1;
        SpeechSlot* second = reinterpret_cast<SpeechSlot*>(self + 0x214) + other;
        if (second->state != 0 && int32_t(second->event.word0 & kIdMask) == id) {
            if (slot != -1) {
                both = 1;
            } else {
                slot = other;
            }
        }
    } else {
        slot = current;
    }
    return SpeechHistory_Add(speechHistory, slot, id, both);
}

// 0x5A0BE0: the listener is in one of three boxes
uint8_t __cdecl Speech_IsListenerInQuietZone() {
    float x = speechListenerX;
    float y = speechListenerY;
    float z = speechListenerZ;
    if (double(x) > 210.0 && double(x) < 220.0 && z > 8.5f && double(y) > -100.0 && y < -95.0f) {
        return 1;
    }
    if (x > -666.666015625f && x < -655.0f && z < 3.0f && y > -330.0f && y < -300.0f) {
        return 1;
    }
    if (x > 310.0f && x < 315.0f && z > 4.0f && double(y) > 207.0 && double(y) < 210.0) {
        return 1;
    }
    return 0;
}

// 0x5A0D20: either speech channel loading or playing
uint8_t __cdecl Speech_AnyChannelBusyImpl() {
    for (int i = 0; i < 2; ++i) {
        void* channel = streamChannelList[i];
        if (Channel_Fn5A6D60(channel) != 0 || Channel_Fn5A6D80(channel) != 0 || Channel_Fn5A6DA0(channel) != 0) {
            return 1;
        }
    }
    return 0;
}

// 0x5A0D90: a filter: shows the subtitle "S<n>" if the text has it (never plays)
uint8_t __cdecl Speech_ShowSubtitle(void*, int number, int duration) {
    char key[0x10];
    Sprintf(key, reinterpret_cast<const char*>(0x009219D8), number);   // "S%d"
    if (Text_Get(TheText, key) != missingText) {
        Subtitles_Add(subtitles, key, duration, 0);
    }
    return 0;
}

// 0x5A0DE0 (thiscall): a slot's stream channel
void* __fastcall Speech_ChannelOf(uint8_t* self, re::Edx, uint8_t* slot) {
    int32_t offset = int32_t(slot - self - 0x214);
    return streamChannelList[offset / 0x5C];
}

// 0x5A0E10 (thiscall, a slot): playing the event (variant -1 any) for the ped (any if null)
uint8_t __fastcall SpeechSlot_IsPlaying(SpeechSlot* self, re::Edx, uint32_t id, uint32_t variant, void* ped) {
    if (self->state != 2 || id != (self->event.word0 & kIdMask)) {
        return 0;
    }
    if (variant != 0xFFFFFFFF && variant != (self->event.word4 & 0x7FF)) {
        return 0;
    }
    if (ped != nullptr && ped != self->ped) {
        return 0;
    }
    return Channel_Fn5A6DA0(ChannelOfSlot(self)) != 0 ? 1 : 0;
}

// 0x5A0E80 (thiscall, a slot): preloads its channel
uint8_t __fastcall SpeechSlot_Preload(SpeechSlot* self, re::Edx, int, float volume, int) {
    if (streamsBlocked != 0) {
        return 0;
    }
    void* channel = ChannelOfSlot(self);
    if (Channel_Fn5A6D60(channel) != 0) {
        return 0;
    }
    return Channel_Preload(channel, nullptr, 0, volume, 2, 0, 1);
}

// ---- the event filters (cdecl (speaker, target, event*), true: say it) ------------------------

inline uint32_t RandTimes(int n) {
    return uint32_t(Rand() * n / 0x7FFF);
}

// 0x5A10B0: a crowd line: by area 0x26, chance or chapter
uint8_t __cdecl SpeechFilter_Crowd(void*, void*, SpeechEvent* ev) {
    uint32_t word = ev->word0;
    At<uint8_t>(ev, 0xA) |= 4;
    ev->timeout = 0x1D4C;
    ev->word0 = (word & 0xFE1677FF) | 0x167600;
    int pick = Rand() * 4 / 0x7FFF;
    if (currArea == 0x26) {
        ev->word0 = (ev->word0 & 0xFFFFF608) | 0x1F608;
        return 1;
    }
    if (pick != 0) {
        ev->word0 = SetId(ev->word0, 0x128);
        return 1;
    }
    switch (chapter) {
    case 0: ev->word0 = SetId(ev->word0, 0x11F); break;
    case 1: ev->word0 = SetId(ev->word0, 0x120); break;
    case 2: ev->word0 = SetId(ev->word0, 0x121); break;
    case 3: ev->word0 = SetId(ev->word0, 0x122); break;
    default: ev->word0 = SetId(ev->word0, 0x123); break;
    }
    return 1;
}

bool Has(int id, uint8_t* ped) {
    return SpeechLib_Has(TheSpeechLib, id, ModelOf(ped)) != 0;
}

// 0x5A1410: a taunt: one of five, some with the reply when the target is in state 2
uint8_t __cdecl SpeechFilter_Taunt(uint8_t* ped, uint8_t* target, SpeechEvent* ev) {
    ev->word0 &= 0xFFFFFE00;
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    int pick = Rand() * 4 / 0x7FFF;
    switch (pick) {
    case 0:
        if (RandomInt(0, 100) < 0x14) {
            ev->word0 ^= ((chapter + 0x52u) ^ ev->word0) & kIdMask;
            if (Has(ev->word0 & kIdMask, ped)) {
                break;
            }
        }
        ev->word0 = SetId(ev->word0, 0x51);
        if (target != nullptr && At<int32_t>(target, 0x3C) == 2 && Has(0x59, ped)) {
            ev->word0 = SetId(ev->word0, 0x59);
        }
        break;
    case 1:
        ev->word0 = SetId(ev->word0, 0x5D);
        break;
    case 2:
    case 4:
        ev->word0 = SetId(ev->word0, pick == 2 ? 0x62 : 0x5F);
        if (target != nullptr && At<int32_t>(target, 0x3C) == 2 && Has(0x63, ped)) {
            ev->word0 = SetId(ev->word0, 0x63);
        }
        break;
    case 3:
        ev->word0 = SetId(ev->word0, 0x5B);
        break;
    default:
        break;
    }
    if ((ev->word0 & kIdMask) == 0x55 && SpeechMission_IsDone(TheSpeech) == 0) {
        ev->word0 = SetId(ev->word0, 0x51);
    }
    return Has(ev->word0 & kIdMask, ped) ? 1 : 0;
}


// 0x5A1620: a line about the player's clothes (reply: the answer's ids, 0x12A lower)
int __cdecl Speech_PickClothingLineImpl(uint8_t* ped, int reply) {
    bool isReply = re::Flag(reply);
    int kind = isReply ? 0 : 2;
    int offset = isReply ? -0x12A : 0;
    if (PlayerWearing_601410(ped, kind, 1) == 0) {
        return isReply ? 0x91 : 0x1B9;
    }
    auto slotKind = [ped](int8_t slot) {
        int32_t clothing = At<int32_t>(ped, 0x1310);
        if (slot == -1 || clothing == -1) {
            return 1;
        }
        return clothingKinds[clothing * 11 + slot];
    };
    int lines[6];
    int count = 0;
    if (slotKind(clothesSlot0) == kind) {
        int id = (outfitCurrent == outfitUniform ? 0x1C3 : 0x1C4) + offset;
        if (Has(id, ped)) {
            count = 1;
            lines[0] = id;
        }
    }
    if (slotKind(clothesSlot1) == kind) {
        int id = 0x1C6 + offset;
        if (Has(id, ped)) {
            lines[count++] = id;
        }
    }
    if (slotKind(clothesSlot2) == kind) {
        int id = 0x1C5 + offset;
        if (Has(id, ped)) {
            lines[count++] = id;
        }
    }
    if (slotKind(clothesSlot3) == kind) {
        int id = 0x1C7 + offset;
        if (Has(id, ped)) {
            lines[count++] = id;
        }
    }
    if (playerDirt > 0 && (playerFlags5F87 & 1) != 0 && Rand() * 99 / 0x7FFF < 0x32) {
        int id = 0x1C8 + offset;
        if (Has(id, ped)) {
            lines[count++] = id;
        }
    }
    if (!isReply && Rand() * 99 / 0x7FFF < (count != 0 ? 0x1E : 0x3C)) {
        int answer = Speech_PickClothingLine(ped, 1);
        if (answer != 0x98 && answer != 0x91 && Has(0x1CB, ped)) {
            return 0x1CB;
        }
    }
    int id = 0x1C2 + offset;
    if (count != 0) {
        if (Has(id, ped)) {
            lines[count++] = id;
        }
        return lines[Rand() * (count - 1) / 0x7FFF];
    }
    if (!Has(id, ped)) {
        id = isReply ? 0x91 : 0x1B9;
    }
    return id;
}

// 0x5A19A0: a filter: priority 9, bumped for peds with +0xA4
uint8_t __cdecl SpeechFilter_Priority9(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    Ped_GetTarget(ped);
    ev->word0 = (ev->word0 & 0xFE13FFFF) | 0x120000;
    if (At<int32_t>(ped, 0xA4) != 0) {
        BumpCounter(ev->wordC);
    }
    At<uint8_t>(ev, 9) |= 2;
    return 1;
}

inline int32_t G(uintptr_t address) {
    return *reinterpret_cast<int32_t*>(address);
}

// 0x5A1A30: the voice a ped answers with (a model id from the model globals); a non-ped
// reads through null like the game
int32_t __cdecl Speech_VoiceFor(uint8_t* entity) {
    uint8_t* ped = entity == nullptr ? nullptr : AsPed(entity);
    int32_t kind = At<int32_t>(ped, 0x1310);
    int32_t model = At<int16_t>(ped, 0x10E);
    if ((kind >= 1 && kind < 7) || kind == 0xB || kind == 0xD) {
        if (Ped_GetModelInfo(ped)[0x50] != 0) {
            int32_t special = G(0x00BF3EAC);
            if (model == G(0x00BF3EA4) || model == G(0x00BF3EA8) || model == special) {
                return special;
            }
            return G(0x00BF40A8);
        }
        int32_t list[8] = {G(0x00BF3EBC), G(0x00BF3EB4), G(0x00BF3EC0), G(0x00BF4030),
                           G(0x00BF4038), G(0x00BF3EF8), G(0x00BF401C), G(0x00BF40BC)};
        bool found = false;
        for (int i = 0; i < 8; ++i) {
            if (SpeechModelMatches(model, uint8_t(list[i])) != 0) {
                found = true;
            }
        }
        if (found) {
            return G(0x00BF3EC0);
        }
        if (SpeechModelMatches(model, uint8_t(G(0x00BF3F48))) != 0) {
            return G(0x00BF3F48);
        }
        if (SpeechModelMatches(model, uint8_t(G(0x00BF3F18))) != 0) {
            return G(0x00BF3F48);
        }
        if (SpeechModelMatches(model, uint8_t(G(0x00BF3F20))) == 0) {
            return G(0x00BF400C);
        }
        return G(0x00BF3F20);
    }
    if (Ped_GetModelInfo(ped)[0x50] != 0) {
        return G(0x00BF3EB0);
    }
    return G(0x00BF3F20);
}

// 0x5A1BA0 (thiscall, this unused): the event's priority from its kind, the distance to
// the camera (under 10) and whether the target is the player
void __fastcall Speech_SetPriorityImpl(void*, re::Edx, uint8_t* ped, uint8_t* target, SpeechEvent* ev) {
    ev->wordC &= ~3u;
    uint32_t priority = PriorityOf(ev->word0);
    if (ped == nullptr) {
        return;
    }
    bool atPlayer = target != nullptr && target == playerPed;
    uint8_t* matrix = At<uint8_t*>(ped, 0x14);
    const CVector* source = reinterpret_cast<const CVector*>(matrix != nullptr ? matrix + 0x30 : ped + 4);
    CVector position = *source;
    CVector delta;
    const CVector* d = VectorSub(&delta, reinterpret_cast<CVector*>(GetLocalTransform(GetCameraNode(nearPlane)) + 0x24),
                                 &position);
    float distance2 = d->y * d->y + d->x * d->x + d->z * d->z;
    bool near = double(distance2) < 100.0;
    bool hasA4 = At<int32_t>(ped, 0xA4) != 0;
    uint32_t word = ev->word0;
    if (ped != playerPed) {
        bool extra = false;
        bool skip = false;
        switch (PriorityOf(word)) {
        case 1: case 3: case 0x10:
            priority = atPlayer ? 0x10 : (near ? 3 : 1);
            break;
        case 2: case 6: case 0x11:
            if (atPlayer) {
                priority = 0x11;
                extra = true;
            } else {
                priority = near ? 6 : 2;
            }
            break;
        case 4: case 10:
            priority = near ? 10 : 4;
            break;
        case 5: case 0xE: case 0x13:
            if (atPlayer) {
                priority = 0x13;
                extra = true;
            } else {
                priority = near ? 0xE : 5;
            }
            break;
        case 7: case 0xC:
            priority = near ? 0xC : 7;
            break;
        case 8: case 0xD:
            priority = near ? 0xD : 8;
            break;
        case 9: case 0xF: case 0x14:
            if (atPlayer) {
                priority = 0x14;
                extra = true;
            } else {
                priority = near ? 0xF : 9;
            }
            break;
        case 0x12:
            extra = true;
            break;
        case 0x15:
            skip = true;
            break;
        default:
            break;
        }
        if (!skip) {
            if (hasA4) {
                BumpCounter(ev->wordC);
            }
            if (extra && !near) {
                BumpCounter(ev->wordC);
            }
        }
    } else if ((word & 0x1FE0000) <= 0x2C0000) {
        priority = 0x16;
    }
    ev->word0 = (((priority << 17) ^ word) & 0x1FE0000) ^ word;
}

// 0x5A1DE0 (thiscall): the event may cut in on the slot (effective priority: priority
// minus the counter)
uint8_t __fastcall Speech_CanInterruptImpl(void*, re::Edx, SpeechSlot* slot, SpeechEvent* ev) {
    int32_t state = slot->state;
    if (state == 0) {
        return 1;
    }
    int32_t mine = int32_t(PriorityOf(slot->event.word0)) - int32_t(slot->event.wordC & 3);
    uint32_t priority = PriorityOf(ev->word0);
    int32_t theirs = int32_t(priority) - int32_t(ev->wordC & 3);
    if (mine >= theirs) {
        return 0;
    }
    if (priority - 1 <= 0xE && priority != 0xB) {
        return 0;
    }
    if (state != 3 || (slot->event.word0 & 0x1FE0000) != 0x160000) {
        return 1;
    }
    if (priority < 0x16) {
        return 0;
    }
    return priority <= 0x18 ? 1 : 0;
}

// 0x5A1E80 (thiscall): the event may cut in on either slot
int32_t __fastcall Speech_CanStartImpl(uint8_t* self, re::Edx, SpeechEvent* ev) {
    int32_t current = At<int32_t>(self, 0x2CC);
    SpeechSlot* slots = reinterpret_cast<SpeechSlot*>(self + 0x214);
    if (Speech_CanInterrupt(self, &slots[current], ev) != 0) {
        return 1;
    }
    if (Speech_CanInterrupt(self, &slots[(current - 1) & 1], ev) != 0) {
        return 1;
    }
    return 0;
}

// 0x5A1EE0 (this unused): the debug dump of an event; what's left without its prints (the
// game's printf is empty): the target's vf 0xAC
void __stdcall Speech_DebugEvent(uint8_t* ped, int) {
    if (ped == nullptr) {
        return;
    }
    Ped_GetModelInfo(ped);
    PedTypeName(At<int32_t>(ped, 0x1310));
    uint8_t* target = Ped_GetTarget(ped);
    if (target == nullptr || AsPed(target) == nullptr) {
        return;
    }
    Ped_GetModelInfo(target);
    if (speechDebugNames != 0) {
        PedTypeName(At<int32_t>(target, 0x1310));
    }
    using Fn = void(__thiscall*)(void*);
    re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(target))[0xAC / 4])(target);
}

// 0x5A20E0: one of a list's entities (a pointer array: count +0x30, size +0x2C, items +4)
// is speaking
uint8_t __stdcall Speech_IsAnySpeaking(uint8_t* list) {
    if (list == nullptr) {
        return 0;
    }
    uint32_t count = At<uint32_t>(list, 0x30);
    int32_t current = speechCurrentSlot;
    for (uint32_t i = 0; i < count; ++i) {
        void* entity = i < At<uint32_t>(list, 0x2C) ? At<void*>(list, 4 + int(i) * 4) : nullptr;
        if (entity == nullptr) {
            return 0;
        }
        SpeechSlot& first = speechSlots[current];
        if (SlotBusy(first) && first.ped == entity) {
            return 1;
        }
        SpeechSlot& second = speechSlots[(current - 1) & 1];
        if (SlotBusy(second) && second.ped == entity) {
            return 1;
        }
    }
    return 0;
}

// 0x5A2170 (thiscall): the ped may speak (the player; else not while +0x1570 is 2 or
// socialising with a partner)
uint8_t __fastcall Speech_PedMaySpeak(uint8_t* self, re::Edx, uint8_t* ped, int) {
    if (ped == nullptr) {
        return 0;
    }
    if (ped == playerPed) {
        return 1;
    }
    if (At<uint8_t>(ped, 0x1570) != 2 && (At<int32_t>(ped, 0xBB0) == 0 || At<int32_t>(ped, 0xBB4) == 0)) {
        return 1;
    }
    At<int32_t>(self, 0xA88) = -0x13;
    return 0;
}

// 0x5A21B0 (thiscall): the event passes the ped's speech flag (+0x1518)
uint8_t __fastcall Speech_PassesPedFlag(uint8_t* self, re::Edx, uint8_t* ped, int id) {
    if (At<uint32_t>(self, 0x2D0 + id * 4) == 0x005A1370) {
        return 1;
    }
    if ((ped == nullptr || At<int32_t>(ped, 0x1518) == 0) && speechAlways == 0) {
        return 1;
    }
    if (ped != nullptr && ped == playerPed) {
        return Speech_IsAllowedForPlayer(id);
    }
    return 0;
}

// 0x5A2200 (thiscall): says an event: its volume from the distance (full for a valid
// entity), queued in the library; the result also goes to +0xA88 (debug prints dropped:
// the game's printf is empty)
int32_t __fastcall Speech_Say(uint8_t* self, re::Edx, SpeechEvent* ev, uint8_t* ped, uint32_t distance) {
    if (Speech_IsEnabled(self, ev) == 0) {
        return -13;
    }
    if (ped != nullptr && AsPed(ped) != nullptr && At<int32_t>(ped, 0x14D8) != 0) {
        return -13;
    }
    uint8_t flags = At<uint8_t>(ev, 8);
    if ((flags & 8) != 0 && ped != nullptr && Speech_IsPedSaying(self, ped, PriorityOf(ev->word0)) != 0) {
        return -18;
    }
    if ((flags & 0x10) != 0) {
        int32_t current = speechCurrentSlot;
        uint32_t priority = PriorityOf(ev->word0);
        SpeechSlot& first = speechSlots[current];
        if (SlotBusy(first) && (priority == 0xFFFFFFFF || priority > PriorityOf(first.event.word0))) {
            return -20;
        }
        // (the second slot compared with the first slot's priority)
        SpeechSlot& second = speechSlots[(current - 1) & 1];
        if (SlotBusy(second) && (priority == 0xFFFFFFFF || priority > PriorityOf(first.event.word0))) {
            return -20;
        }
    }
    uint8_t* valid = IsEntityPointerValid(ped) != 0 ? ped : nullptr;
    uint8_t* record = Speech_AllocRecord(self);
    if (record == nullptr) {
        At<int32_t>(self, 0xA88) = -8;
        return -8;
    }
    At<uint32_t>(record, 4) ^= (At<uint32_t>(record, 4) ^ distance) & 0x7F;
    if (valid != nullptr) {
        At<float>(record, 0) = 1.0f;
        SetRef(record + 0xC, valid);
    } else {
        uint32_t level = uint32_t(SFX_Fn59E3E0(0.0f, int(distance)));
        float volume = float(double(level) * 0.0009765625);
        At<float>(record, 0) = volume;
        if (Speech_IsLoud(*ev) != 0) {
            float scale = float(speechLoudScale / 100.0);
            At<float>(record, 0) = volume * scale;
        }
        SetRef(record + 0xC, nullptr);
    }
    if (double(At<float>(record, 0)) <= 0.1) {
        At<uint8_t>(record, 8) &= 0xFE;   // (too quiet)
        return 0;
    }
    int32_t result = SpeechLib_Queue(TheSpeechLib, ev, record);
    if (result != 0) {
        At<uint8_t>(record, 8) &= 0xFE;
    }
    At<int32_t>(self, 0xA88) = result;
    return result;
}

// 0x5A25B0 (thiscall): whether the event may start: 0 no, 1 already said by that speaker,
// 2 yes, 3 a repeat of what plays
int32_t __fastcall Speech_CheckStart(uint8_t* self, re::Edx, SpeechEvent* ev, uint8_t* other) {
    if (Speech_CanStart(self, ev) == 0) {
        return 2;
    }
    SpeechSlot* mine = reinterpret_cast<SpeechSlot*>(self + 0x214);
    uint8_t flags9 = At<uint8_t>(ev, 9);
    int32_t current = speechCurrentSlot;
    if ((flags9 & 2) != 0) {
        bool repeat = false;
        SpeechSlot& first = speechSlots[current];
        if (first.state != 0 && ((first.event.word0 ^ ev->word0) & 0x1FE00) == 0 &&
            ((mine[At<int32_t>(self, 0x2CC)].event.word0 ^ ev->word0) & kIdMask) == 0) {
            repeat = true;
        }
        SpeechSlot& second = speechSlots[(current - 1) & 1];
        if (second.state != 0 && ((second.event.word0 ^ ev->word0) & 0x1FE00) == 0 &&
            ((mine[(At<int32_t>(self, 0x2CC) - 1) & 1].event.word0 ^ ev->word0) & kIdMask) == 0) {
            return 3;
        }
        if (repeat) {
            return 3;
        }
    }
    if (other != nullptr) {
        SpeechSlot& first = speechSlots[current];
        int32_t state = first.state;
        bool busy = false;
        if (state != 0 && first.ped == At<void*>(other, 0xC)) {
            int32_t order = SpeechEvent_Compare(self, &mine[At<int32_t>(self, 0x2CC)].event, ev);
            if (order < 0 || state != 3) {
                busy = true;
            }
        }
        SpeechSlot& second = speechSlots[(current - 1) & 1];
        if (second.state != 0 && second.ped == At<void*>(other, 0xC)) {
            int32_t order = SpeechEvent_Compare(self, &mine[(At<int32_t>(self, 0x2CC) - 1) & 1].event, ev);
            if (order < 0 || second.state != 3) {
                busy = true;
            }
        }
        if (busy && (At<uint8_t>(ev, 0xA) & 1) == 0) {
            return 0;
        }
        if ((At<uint8_t>(ev, 8) & 0x88) != 0 || (flags9 & 5) != 0) {
            void* speaker = At<void*>(other, 0xC);
            if (state != 0 && state != 4 && (speaker == nullptr || first.ped == speaker)) {
                return 1;
            }
            if (SlotBusy(second) && (speaker == nullptr || second.ped == speaker)) {
                return 1;
            }
        }
    }
    int32_t index = At<int32_t>(self, 0x2CC);
    SpeechSlot* slot = &mine[index];
    if (slot->state == 0 && Channel_IsIdle(ChannelOfSlot(slot)) != 0) {
        return 0;
    }
    if (slot->state != 0 && SpeechEvent_Compare(self, &slot->event, ev) < 0 && (At<uint8_t>(ev, 9) & 8) != 0) {
        return 0;
    }
    if ((At<uint8_t>(ev, 8) & 1) == 0 && At<uint8_t>(self, 0xB1) == 0) {
        return 2;
    }
    slot = &mine[(index - 1) & 1];
    if (slot->state == 0 && Channel_IsIdle(ChannelOfSlot(slot)) != 0) {
        return 0;
    }
    slot = &mine[(At<int32_t>(self, 0x2CC) - 1) & 1];
    if (slot->state == 0) {
        return 2;
    }
    if (SpeechEvent_Compare(self, &slot->event, ev) < 0 && (At<uint8_t>(ev, 9) & 8) != 0) {
        return 0;
    }
    return 2;
}

// 0x5A28A0: a record's done callback (+0x14, once) with the speaker (a ped or null) and
// its data (+0x18); the event's queued flag goes off (reads +0xC even when it's null)
void __fastcall Speech_RunDoneCallback(void*, re::Edx, uint8_t* record, uint8_t* ev) {
    if (ev != nullptr) {
        At<uint8_t>(ev, 8) &= 0xFE;
    }
    using Callback = void(__cdecl*)(void*, void*);
    Callback callback = At<Callback>(record, 0x14);
    if (callback == nullptr) {
        return;
    }
    At<Callback>(record, 0x14) = nullptr;
    uint8_t* speaker = At<uint8_t*>(ev, 0xC);
    if (speaker == nullptr) {
        callback(nullptr, At<void*>(record, 0x18));
        return;
    }
    callback(AsPed(speaker), At<void*>(record, 0x18));
}

// 0x5A2900: the ped is saying the event (-1 any id or variant); without a ped: any speech
uint8_t __cdecl Speech_IsSaying(uint8_t* ped, int32_t id, int32_t variant) {
    if (ped != nullptr) {
        int32_t current = speechCurrentSlot;
        SpeechSlot* slots[2] = {&speechSlots[current], &speechSlots[(current - 1) & 1]};
        for (SpeechSlot* slot : slots) {
            if (slot->state != 0 && slot->ped == ped &&
                (id == -1 || int32_t(slot->event.word0 & kIdMask) == id) &&
                (variant == -1 || int32_t(slot->event.word4 & 0x7FF) == variant)) {
                return 1;
            }
        }
        return SpeechLib_IsSaying(TheSpeechLib, ped, id, variant) != 0 ? 1 : 0;
    }
    if (Speech_AnyChannelBusy() != 0) {
        return 1;
    }
    int32_t current = speechCurrentSlot;
    if (speechSlots[current].state != 0 || speechSlots[(current - 1) & 1].state != 0) {
        return 1;
    }
    return SpeechLib_IsSaying(TheSpeechLib, nullptr, -1, -1) != 0 ? 1 : 0;
}

uint8_t* TargetPed(uint8_t* ped) {
    uint8_t* target = Ped_GetTarget(ped);
    return target == nullptr ? nullptr : AsPed(target);
}

// 0x5A2A00: a filter: priority 6, 1 s
uint8_t __cdecl SpeechFilter_Priority6(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    ev->word0 = (ev->word0 & 0xFE0DFFFF) | 0xC0000;
    ev->timeout = 1000;
    Speech_SetPriority(TheSpeech, ped, TargetPed(ped), ev);
    ev->word4 &= 0xFFFFF800;
    return 1;
}

inline void SetModel(SpeechEvent* ev, uint8_t* ped) {
    ev->word0 ^= ((uint32_t(int32_t(ModelOf(ped))) << 9) ^ ev->word0) & 0x1FE00;
}

// 0x5A2BA0: a filter: the speaker's model, priority 0xF, 1 s
uint8_t __cdecl SpeechFilter_Priority15(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    SetModel(ev, ped);
    ev->timeout = 1000;
    ev->word0 = (ev->word0 & 0xFE1FFFFF) | 0x1E0000;
    Speech_SetPriority(TheSpeech, ped, TargetPed(ped), ev);
    ev->word4 &= 0xFFFFF800;
    At<uint8_t>(ev, 9) &= 0xF;
    return 1;
}

// 0x5A2C30: a filter: the speaker's model, priority 0x15, 7.5 s, urgent
uint8_t __cdecl SpeechFilter_Priority21(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    uint8_t flags = At<uint8_t>(ev, 8);
    At<uint8_t>(ev, 9) |= 4;
    SetModel(ev, ped);
    ev->timeout = 0x1D4C;
    At<uint8_t>(ev, 8) = uint8_t((flags & 0x73) | 4);
    ev->word0 = (ev->word0 & 0xFE2BFFFF) | 0x2A0000;
    Speech_SetPriority(TheSpeech, ped, TargetPed(ped), ev);
    return 1;
}

// 0x5A2CC0, 0x5A2DB0, 0x5A2EA0: a fight line at the target: the attitude one (3..4), else by
// chance, else 0xA9; priority 0x13
uint8_t FightLine(uint8_t* ped, SpeechEvent* ev, uint32_t byAttitude, uint32_t byChance) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    SetModel(ev, ped);
    ev->timeout = 0x1D4C;
    if (PedFlags_Fn48EBA0(ped + 0xB44) != 0) {
        At<uint8_t>(ev, 9) &= 0xF;
    }
    uint8_t* target = Ped_GetTarget(ped);
    uint8_t* at = nullptr;
    bool chosen = false;
    if (target != nullptr && AsPed(target) != nullptr) {
        at = target;
        int32_t attitude = 0;
        attitude = *Ped_GetAttitude(ped, &attitude, target);
        if (attitude >= 3 && attitude <= 4) {
            ev->word0 = SetId(ev->word0, byAttitude);
            chosen = true;
        }
    }
    if (!chosen) {
        ev->word0 = SetId(ev->word0, Rand() < 0x3FFF ? byChance : 0xA9);
    }
    ev->word0 = (ev->word0 & 0xFE27FFFF) | 0x260000;
    Speech_SetPriority(TheSpeech, ped, at, ev);
    return 1;
}

uint8_t __cdecl SpeechFilter_Fight1CE(uint8_t* ped, void*, SpeechEvent* ev) {
    return FightLine(ped, ev, 0x1CE, 0x1CE);
}
uint8_t __cdecl SpeechFilter_Fight1CC(uint8_t* ped, void*, SpeechEvent* ev) {
    return FightLine(ped, ev, 0x1CC, 0x1CC);
}
uint8_t __cdecl SpeechFilter_FightAD(uint8_t* ped, void*, SpeechEvent* ev) {
    return FightLine(ped, ev, 0xAD, 0xAC);
}

// ---- registration ---------------------------------------------------------------------------

#define SPEECHMGR_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(speechmgr_original_, __LINE__);                                       \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(speechmgr_original_, __LINE__), \
             re::Mode::Original)

SPEECHMGR_REVERSED("SpeechItems::Init", 0x0059FC20, &SpeechItems_InitImpl);
SPEECHMGR_REVERSED("SpeechLib::Shutdown", 0x0059FC90, &SpeechLib_Shutdown);
SPEECHMGR_REVERSED("SpeechLib::Queue", 0x0059FCC0, &SpeechLib_QueueImpl);
SPEECHMGR_REVERSED("SpeechLib::SpeechLib", 0x0059FD50, &SpeechLib_Construct);
SPEECHMGR_REVERSED("SpeechEvent::IsCalm", 0x0059FE00, &SpeechEvent_IsCalm);
SPEECHMGR_REVERSED("Speech::IsStreamLoaded", 0x0059FE20, &Speech_IsStreamLoaded);
SPEECHMGR_REVERSED("Speech::IsSpecialModel", 0x0059FE40, &Speech_IsSpecialModel);
SPEECHMGR_REVERSED("Speech::PlayerHasWeaponLine", 0x0059FEB0, &Speech_PlayerHasWeaponLine);
// (callers rely on edx: 0x5A1299 keeps a pointer in edx across it; the game's version
// doesn't touch edx: switch to ours only with a wrapper that preserves it)
SPEECHMGR_REVERSED("SpeechMission::IsDone", 0x0059FEE0, &SpeechMission_IsDoneImpl);
SPEECHMGR_REVERSED("SpeechMission::IsFree", 0x0059FF80, &SpeechMission_IsFree);
SPEECHMGR_REVERSED("Speech::IsLoud", 0x005A0530, &Speech_IsLoudImpl);
SPEECHMGR_REVERSED("Speech::ClearRefs", 0x005A0570, &Speech_ClearRefs);
SPEECHMGR_REVERSED("Speech::CompareDifferences", 0x005A0780, &CompareDifferences);
SPEECHMGR_REVERSED("Speech::AllocRecord", 0x005A07B0, &Speech_AllocRecordImpl);
SPEECHMGR_REVERSED("Speech::IsAllowedForPlayer", 0x005A0810, &Speech_IsAllowedForPlayerImpl);
SPEECHMGR_REVERSED("Speech::IsPedSaying", 0x005A0850, &Speech_IsPedSayingImpl);
SPEECHMGR_REVERSED("Speech::IsPedSayingEvent", 0x005A08F0, &Speech_IsPedSayingEvent);
SPEECHMGR_REVERSED("Speech::ResetFilter", 0x005A09A0, &Speech_ResetFilter);
SPEECHMGR_REVERSED("Speech::IsEnabled", 0x005A09B0, &Speech_IsEnabledImpl);
SPEECHMGR_REVERSED("Speech::ParseNumber", 0x005A0A70, &Speech_ParseNumber);
SPEECHMGR_REVERSED("Speech::ForgetPed", 0x005A0AB0, &Speech_ForgetPed);
SPEECHMGR_REVERSED("Speech::RecordHistory", 0x005A0B10, &Speech_RecordHistory);
SPEECHMGR_REVERSED("Speech::IsListenerInQuietZone", 0x005A0BE0, &Speech_IsListenerInQuietZone);
SPEECHMGR_REVERSED("Speech::AnyChannelBusy", 0x005A0D20, &Speech_AnyChannelBusyImpl);
SPEECHMGR_REVERSED("Speech::ShowSubtitle", 0x005A0D90, &Speech_ShowSubtitle);
SPEECHMGR_REVERSED("Speech::ChannelOf", 0x005A0DE0, &Speech_ChannelOf);
SPEECHMGR_REVERSED("SpeechSlot::IsPlaying", 0x005A0E10, &SpeechSlot_IsPlaying);
SPEECHMGR_REVERSED("SpeechSlot::Preload", 0x005A0E80, &SpeechSlot_Preload);
SPEECHMGR_REVERSED("SpeechFilter::Crowd", 0x005A10B0, &SpeechFilter_Crowd);
SPEECHMGR_REVERSED("SpeechFilter::Taunt", 0x005A1410, &SpeechFilter_Taunt);
SPEECHMGR_REVERSED("Speech::PickClothingLine", 0x005A1620, &Speech_PickClothingLineImpl);
SPEECHMGR_REVERSED("SpeechFilter::Priority9", 0x005A19A0, &SpeechFilter_Priority9);
SPEECHMGR_REVERSED("Speech::VoiceFor", 0x005A1A30, &Speech_VoiceFor);
SPEECHMGR_REVERSED("Speech::SetPriority", 0x005A1BA0, &Speech_SetPriorityImpl);
SPEECHMGR_REVERSED("Speech::CanInterrupt", 0x005A1DE0, &Speech_CanInterruptImpl);
SPEECHMGR_REVERSED("Speech::CanStart", 0x005A1E80, &Speech_CanStartImpl);
SPEECHMGR_REVERSED("Speech::DebugEvent", 0x005A1EE0, &Speech_DebugEvent);
SPEECHMGR_REVERSED("Speech::IsAnySpeaking", 0x005A20E0, &Speech_IsAnySpeaking);
SPEECHMGR_REVERSED("Speech::PedMaySpeak", 0x005A2170, &Speech_PedMaySpeak);
SPEECHMGR_REVERSED("Speech::PassesPedFlag", 0x005A21B0, &Speech_PassesPedFlag);
SPEECHMGR_REVERSED("Speech::Say", 0x005A2200, &Speech_Say);
SPEECHMGR_REVERSED("Speech::CheckStart", 0x005A25B0, &Speech_CheckStart);
SPEECHMGR_REVERSED("Speech::RunDoneCallback", 0x005A28A0, &Speech_RunDoneCallback);
SPEECHMGR_REVERSED("Speech::IsSaying", 0x005A2900, &Speech_IsSaying);
SPEECHMGR_REVERSED("SpeechFilter::Priority6", 0x005A2A00, &SpeechFilter_Priority6);
SPEECHMGR_REVERSED("SpeechFilter::Priority15", 0x005A2BA0, &SpeechFilter_Priority15);
SPEECHMGR_REVERSED("SpeechFilter::Priority21", 0x005A2C30, &SpeechFilter_Priority21);
SPEECHMGR_REVERSED("SpeechFilter::Fight1CE", 0x005A2CC0, &SpeechFilter_Fight1CE);
SPEECHMGR_REVERSED("SpeechFilter::Fight1CC", 0x005A2DB0, &SpeechFilter_Fight1CC);
SPEECHMGR_REVERSED("SpeechFilter::FightAD", 0x005A2EA0, &SpeechFilter_FightAD);

} // namespace
