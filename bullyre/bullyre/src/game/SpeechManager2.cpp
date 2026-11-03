// The speech manager, continued (0x5A3370..0x5A5DEC): more event filters, the filter
// table's setup, adding events (Speech_Add and its scripted and variant forms), the
// constructor, and the two playing slots' state machine (0 idle, 1 loading, 2 preloaded,
// 3 playing, 4 finishing) with their per-frame update. See SpeechManager.cpp for the
// event and slot layouts.

#include <cmath>
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
    uint32_t word0;      // +0 id (0..8) / model (9..16) / priority (17..24)
    uint32_t word4;      // +4 variant (0..10)
    uint32_t flags8;     // +8
    uint32_t wordC;      // +0xC
    uint32_t timeout;    // +0x10
    uint32_t unknown14[2];
    uint32_t flags1C;    // +0x1C (bit 0: the position below is set)
    CVector position;    // +0x20
    int32_t unknown2C;   // +0x2C
};
static_assert(sizeof(SpeechEvent) == 0x30);

// a playing slot (0x5C bytes, two at 0xCF458C)
struct SpeechSlot {
    int32_t state;       // +0 (0 idle, 1 loading, 2 preloaded, 3 playing, 4 finishing)
    float volume;        // +4
    uint32_t distance;   // +8 (low 7 bits, signed)
    uint32_t flagsC;     // +0xC
    re::Ptr32<void> ped;           // +0x10 (a registered reference)
    uint32_t unknown14;
    SpeechEvent event;   // +0x18
    re::Ptr32<void> history;       // +0x48
    int32_t historyId;   // +0x4C
    int32_t line;        // +0x50
    uint32_t timeout;    // +0x54
    int32_t subtitle;    // +0x58
};
static_assert(sizeof(SpeechSlot) == 0x5C);

inline uint8_t* const TheSpeech = reinterpret_cast<uint8_t*>(0x00CF4378);
inline uint8_t* const TheSpeechLib = reinterpret_cast<uint8_t*>(0x00CF4E10);
inline void* const lastSpeakerRef = reinterpret_cast<void*>(0x00CF4E08);
inline void* const speechHistory = reinterpret_cast<void*>(0x0208B9F0);
inline void* const TheMissions = reinterpret_cast<void*>(0x020C3CA0);
inline void* const playerClothes = reinterpret_cast<void*>(0x020C5F70);
GAME_ARRAY(SpeechSlot, speechSlots, 0x00CF458C);
GAME_VAR(int32_t, speechCurrentSlot, 0x00CF4644);
GAME_ARRAY(void*, streamChannelList, 0x00CF4290);
GAME_ARRAY(uint8_t, speechInMemory, 0x0208D124);    // per event id: resident (no streaming)
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(void*, population, 0x00C2C108);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_VAR(uint8_t, speechBlockedA, 0x020C5BE6);
GAME_VAR(uint8_t, streamsBlocked, 0x00CF4286);
GAME_VAR(uint8_t, speechMuted, 0x00CF3DD5);
GAME_VAR(uint8_t, speechSubtitles, 0x00CF4429);
GAME_VAR(uint8_t, speechDelayStart, 0x00CF442B);
GAME_VAR(int32_t, speechLock, 0x00CF41F0);
GAME_VAR(uint32_t, lastGreetingTime, 0x00CF6C78);
GAME_VAR(uint8_t*, lastGreetingPed, 0x00CF6C7C);
GAME_VAR(int32_t, mission509, 0x00AE80C0);           // (-1 until looked up)
GAME_VAR(int32_t, mission6B, 0x00AE80BC);

inline void* __cdecl Memset(void* p, int value, uint32_t size) { return crt::Memset(p, value, (size_t)size); }
inline int __cdecl Rand() { return crt::Rand(); }
GAME_FN(int, __cdecl, RandomInt, 0x0040DA80, int low, int high);
GAME_FN(uint8_t, __cdecl, RandomChance_41A5E0, 0x0041A5E0);
GAME_FN(void*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, uint32_t* self);
GAME_FN(int32_t, __thiscall, Missions_Find, 0x006A9FD0, void* missions, uint32_t* key);
GAME_FN(uint8_t*, __thiscall, Missions_GetStats, 0x006AA7C0, void* missions, int32_t mission);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, void* ref, void* entity);
GAME_FN(uint8_t*, __thiscall, Ped_GetModelInfo, 0x00477DC0, void* ped);
GAME_FN(uint8_t*, __thiscall, Ped_GetTarget, 0x00497100, void* ped);
GAME_FN(uint8_t*, __thiscall, Ped_46ECE0, 0x0046ECE0, void* ped);
GAME_FN(int32_t*, __thiscall, Ped_GetAttitudeTo, 0x00475380, void* ped, int32_t* out, int32_t pedType);
GAME_FN(uint8_t, __thiscall, PedFlags_Fn48EBA0, 0x0048EBA0, void* flags);
GAME_FN(uint8_t, __thiscall, PedTask_Fn4864E0, 0x004864E0, void* tasks);
GAME_FN(uint8_t, __thiscall, Population_Fn49E410, 0x0049E410, void* population, void* ped);
GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, void* placeable, CVector* out);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(int32_t, __thiscall, Clothes_Fn6CB8F0, 0x006CB8F0, void* clothes);
GAME_FN(uint8_t*, __cdecl, EntityAsPed, 0x00468E10, void* entity);
GAME_FN(uint8_t, __thiscall, Entity_GetIsOnScreen, 0x00467460, void* entity);
GAME_FN(void, __cdecl, RequestModel, 0x00532020, int id, int flags);
GAME_FN(CVector*, __cdecl, GetCameraPos, 0x00595630, CVector* out);
GAME_FN(uint8_t, __cdecl, ProcessLineOfSight, 0x00461EA0, const CVector* start, const CVector* end, void* colPoint,
        re::Ptr32<void>* entity, int flags, int, int);
GAME_FN(void, __thiscall, ColPoint_Construct, 0x00415FA0, void* colPoint);
GAME_FN(void, __thiscall, ColPoint_Destruct, 0x00415500, void* colPoint);
GAME_FN(void, __cdecl, Fn5B25E0, 0x005B25E0, void* ped, int id, int variant, int);
GAME_FN(void, __cdecl, Fn619A50, 0x00619A50);
GAME_FN(void, __cdecl, Sound_SetVolume5AF6B0, 0x005AF6B0, int32_t handle, float volume);
GAME_FN(void, __thiscall, Ped_Fn68EBB0, 0x0068EBB0, void* ped, int on);
GAME_FN(void, __thiscall, History_Fn68E3C0, 0x0068E3C0, void* history, int32_t id, void* ped);
GAME_FN(void, __thiscall, History_Fn68DE60, 0x0068DE60, void* history, int32_t id);
GAME_FN(void, __thiscall, History_Fn68DDD0, 0x0068DDD0, void* history, int32_t slot);
GAME_FN(void, __thiscall, History_Fn68DCD0, 0x0068DCD0, void* history, int32_t slot);
GAME_FN(void, __thiscall, History_Fn68DDA0, 0x0068DDA0, void* history, int32_t slot);
GAME_FN(void, __thiscall, History_Fn68DCA0, 0x0068DCA0, void* history);
GAME_FN(int32_t, __thiscall, History_Fn68E250, 0x0068E250, void* history, int32_t slot, int32_t line, void* ped);
GAME_FN(uint8_t*, __thiscall, History_Fn68DD30, 0x0068DD30, void* history, int32_t slot, int32_t line, int, int);
GAME_FN(int32_t, __thiscall, Channel_IsIdle, 0x0058E060, void* channel);
GAME_FN(int32_t, __thiscall, Channel_Fn5A6DA0, 0x005A6DA0, void* channel);
GAME_FN(void, __thiscall, Channel_SetPosition, 0x005A6CE0, void* channel, const CVector* position, int distance);
GAME_FN(void, __thiscall, Channel_SetNoPosition, 0x005A6D20, void* channel, uintptr_t, int);
GAME_FN(void, __thiscall, Channel_Play, 0x005A6DB0, void* channel);
GAME_FN(void, __thiscall, Channel_Reset, 0x005A6EA0, void* channel);
GAME_FN(void, __thiscall, Channel_SetVolume, 0x005A6ED0, void* channel, float volume, int ms);
GAME_FN(void, __thiscall, Channel_Stop, 0x005A7140, void* channel, int ms);
GAME_FN(void, __thiscall, SpeechLib_Fn59F8A0, 0x0059F8A0, void* lib);
GAME_FN(void, __thiscall, SpeechLib_Stop59F8D0, 0x0059F8D0, void* lib, int model, int reason, int);
GAME_FN(void, __thiscall, SpeechLib_Stop59F960, 0x0059F960, void* lib, int model, int id, int variant);
GAME_FN(uint8_t, __thiscall, SpeechLib_Has, 0x0059F3E0, void* lib, int id, int model);
// SpeechManager.cpp's
GAME_FN(uint8_t, __thiscall, SpeechEvent_IsCalm, 0x0059FE00, void* ev);
GAME_FN(uint8_t, __cdecl, Speech_IsStreamLoaded, 0x0059FE20, int index);
GAME_FN(uint8_t, __cdecl, Speech_IsSpecialModel, 0x0059FE40, int32_t model);
GAME_FN(uint8_t, __thiscall, Speech_RecordHistory, 0x005A0B10, void* speech, int32_t id);
GAME_FN(uint8_t, __cdecl, Speech_IsListenerInQuietZone, 0x005A0BE0);
GAME_FN(uint8_t, __cdecl, Speech_ShowSubtitle, 0x005A0D90, uintptr_t, int number, int duration);
GAME_FN(void*, __thiscall, Speech_ChannelOf, 0x005A0DE0, void* speech, void* slot);
GAME_FN(uint8_t, __thiscall, SpeechSlot_Preload, 0x005A0E80, void* slot, int line, float volume, void* ped);
GAME_FN(uint8_t, __cdecl, SpeechFilter_Taunt, 0x005A1410, void* ped, void* target, SpeechEvent* ev);
GAME_FN(int, __cdecl, Speech_PickClothingLine, 0x005A1620, void* ped, int reply);
GAME_FN(int32_t, __cdecl, Speech_VoiceFor, 0x005A1A30, void* ped);
GAME_FN(void, __thiscall, Speech_SetPriority, 0x005A1BA0, void* speech, void* ped, void* target, SpeechEvent* ev);
GAME_FN(void, __stdcall, Speech_DebugEvent, 0x005A1EE0, void* ped, int id);
GAME_FN(uint8_t, __stdcall, Speech_IsAnySpeaking, 0x005A20E0, void* list);
GAME_FN(uint8_t, __thiscall, Speech_PedMaySpeak, 0x005A2170, void* speech, void* ped, void* ev);
GAME_FN(int32_t, __thiscall, Speech_Say, 0x005A2200, void* speech, SpeechEvent* ev, void* ped, uint32_t distance);
// this file's
GAME_FN(void, __thiscall, Speech_InitFilters, 0x005A4630, void* speech);
GAME_FN(SpeechEvent*, __thiscall, Speech_Add, 0x005A4920, void* speech, SpeechEvent* out, void* ped, void* target,
        int, int id, int32_t arg6, int32_t arg7, int flag8, int hasPosition, int32_t arg10, const CVector* position);
GAME_FN(SpeechEvent*, __thiscall, Speech_AddScripted, 0x005A4AD0, void* speech, SpeechEvent* out, int id,
        uint32_t variant, void* ped, uint32_t distance, int32_t arg6, int32_t arg7, int urgent, int flag9, int calm);
GAME_FN(void, __thiscall, SpeechSlot_SetState, 0x005A5060, void* slot, int32_t state);
GAME_FN(uint8_t, __thiscall, Speech_CanInterrupt2, 0x005A1DE0, void* speech, SpeechSlot* slot, SpeechEvent* ev);
GAME_FN(int32_t, __thiscall, SpeechEvent_Compare2, 0x0059E7B0, void* speech, SpeechEvent* a, SpeechEvent* b);
GAME_FN(int32_t, __cdecl, LoadFile_42D180, 0x0042D180, const char* path, void* buffer, int size, const char* mode);
GAME_FN(void, __thiscall, SpeechSlot_Update, 0x005A57A0, void* slot);
GAME_FN(uint8_t, __cdecl, Speech_PlayerHasWeaponLine, 0x0059FEB0);
GAME_FN(uint8_t, __cdecl, VectorNotEqual, 0x004305F0, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
// the filters' own state
GAME_VAR(uint32_t, lastAlertTime, 0x00CF4430);
GAME_VAR(uint32_t, lastNoticeTime, 0x00CF6C74);
GAME_VAR(uint8_t*, lastSeenPed, 0x00CF6C80);
GAME_VAR(uint32_t, lastSeenTime, 0x00CF6C84);
GAME_ARRAY(float, lastSeenPos, 0x00CF6C88);        // (a function-local static CVector)
GAME_VAR(uint32_t, lastSeenPosGuard, 0x00CF6C94);
GAME_VAR(uint32_t, lastCommentTime, 0x00CF6C98);    // (never set by its filter)
GAME_ARRAY(float, unusedPos, 0x00CF6C9C);           // (a static CVector nothing reads)
GAME_VAR(uint32_t, unusedPosGuard, 0x00CF6CA8);

uint8_t* AsPed(uint8_t* entity) {
    if (entity == nullptr || (At<uint8_t>(entity, 0x108) & 7) != 3) {
        return nullptr;
    }
    return entity;
}
uint8_t* TargetPed(uint8_t* ped) {
    uint8_t* target = Ped_GetTarget(ped);
    return target == nullptr ? nullptr : AsPed(target);
}

constexpr uint32_t kIdMask = 0x1FF;
inline uint32_t SetId(uint32_t word, uint32_t id) {
    return (word & ~kIdMask) | id;
}
inline int16_t ModelOf(uint8_t* ped) {
    return At<int16_t>(ped, 0x10E);
}
inline void SetModel(SpeechEvent* ev, uint8_t* ped) {
    ev->word0 ^= ((uint32_t(int32_t(ModelOf(ped))) << 9) ^ ev->word0) & 0x1FE00;
}
bool Has(int id, int model) {
    return SpeechLib_Has(TheSpeechLib, id, model) != 0;
}
inline uint32_t PriorityOf(uint32_t word) {
    return (word >> 17) & 0xFF;
}
const CVector* PositionOf(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return reinterpret_cast<const CVector*>(matrix != nullptr ? matrix + 0x30 : entity + 4);
}
int32_t FindMission(const char* name) {
    uint32_t key;
    NameKey_Construct(&key, name);
    int32_t mission = Missions_Find(TheMissions, &key);
    NameKey_Destruct(&key);
    return mission;
}
inline int32_t SignExtend7(uint32_t value) {
    return int32_t(value << 25) >> 25;
}

// ---- more filters (cdecl (speaker, target, event*)) ----------------------------------------

// 0x5A3370: a reply to what the other said (its event at +0x48): the matching answer, a
// taunt for the rest; 0x4F (none) when the other is still speaking or lacks it
uint8_t __cdecl SpeechFilter_Reply(uint8_t* ped, uint8_t* other, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr || PedTask_Fn4864E0(playerPed + 0x668) != 0) {
        return 0;
    }
    if (Speech_IsAnySpeaking(other) != 0) {
        ev->word0 = SetId(ev->word0, 0x4F);
        return 0;
    }
    SpeechEvent said;
    std::memcpy(&said, other + 0x48, sizeof(said));
    switch (said.word0 & kIdMask) {
    case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x59:
        ev->word0 = SetId(ev->word0, 0x5A);
        break;
    case 0x5A: case 0x5C: case 0x61:
        ev->word0 = SetId(ev->word0, 0x4E);
        break;
    case 0x5B: case 0x5D:
        ev->word0 = SetId(ev->word0, 0x5C);
        break;
    case 0x5F:
        ev->word0 = SetId(ev->word0, 0x61);
        break;
    case 0x62: case 0x63:
        ev->word0 = SetId(ev->word0, 0x64);
        break;
    default:   // 0x58, 0x5E, 0x60 and the others
        if (SpeechFilter_Taunt(ped, other, ev) == 0) {
            return 0;
        }
        break;
    }
    if (!Has(ev->word0 & kIdMask, ModelOf(ped))) {
        ev->word0 = SetId(ev->word0, 0x4F);   // (a debug print)
        return 0;
    }
    ev->word0 = (ev->word0 & 0xFE1BFFFF) | 0x1A0000;
    ev->timeout = 0x2EE0;
    Speech_SetPriority(TheSpeech, ped, Ped_46ECE0(ped), ev);
    return 1;
}

// 0x5A3560: a filter: line 0x5E, priority 0xD, 12 s
uint8_t __cdecl SpeechFilter_Line5E(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr || PedTask_Fn4864E0(playerPed + 0x668) != 0) {
        return 0;
    }
    ev->word0 = (ev->word0 & 0xFE1BFE5E) | 0x1A005E;
    ev->timeout = 0x2EE0;
    Speech_SetPriority(TheSpeech, ped, nullptr, ev);
    return 1;
}

// 0x5A35C0: a greeting (0x15) or its variants by attitude, facing, the target's vehicle and
// the season; not the same speaker twice within 8 s unless one side is kind 13
uint8_t __cdecl SpeechFilter_Greeting(uint8_t* ped, uint8_t* target, SpeechEvent* ev) {
    uint32_t now = m_snTimeInMilliseconds;
    if (mission509 == -1) {
        mission509 = FindMission(reinterpret_cast<const char*>(0x00921930));   // "5_09"
    }
    if (mission6B == -1) {
        mission6B = FindMission(reinterpret_cast<const char*>(0x0091DFD0));    // "6_B"
    }
    bool between = false;   // 5_09 done, 6_B not
    if (At<int16_t>(Missions_GetStats(TheMissions, mission509), 2) != 0) {
        between = At<int16_t>(Missions_GetStats(TheMissions, mission6B), 2) == 0;
    }
    uint32_t word = (ev->word0 & 0xFFFFFE15) | 0x15;
    ev->word0 = word;
    At<uint8_t>(ev, 9) = uint8_t((At<uint8_t>(ev, 9) & 0xD) | 2);
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    if (target == nullptr) {
        return 1;
    }
    ev->timeout = 0x1D4C;
    uint8_t* other = AsPed(target);
    ev->word0 = (word & 0xFE07FFFF) | 0x60000;
    Speech_SetPriority(TheSpeech, ped, other, ev);
    if (other == nullptr) {
        return 1;
    }
    int chance = 100;
    bool kind13 = false;
    if (At<int32_t>(other, 0x1310) == 0xD || At<int32_t>(ped, 0x1310) == 0xD) {
        kind13 = true;
    } else {
        if (Speech_PedMaySpeak(TheSpeech, ped, ev) == 0) {
            return 0;
        }
        if (Ped_GetModelInfo(ped)[0x50] != 0) {
            chance = Ped_GetModelInfo(other)[0x50] != 0 ? 0x19 : 0x23;
        } else {
            chance = Ped_GetModelInfo(other)[0x50] != 0 ? 5 : 0x32;
        }
    }
    if (RandomInt(0, 100) >= chance) {
        return 0;
    }
    if (PedTask_Fn4864E0(ped + 0x668) != 0 || PedTask_Fn4864E0(other + 0x668) != 0 || speechLock != 0) {
        lastGreetingTime = now;
        lastGreetingPed = ped;
        return 0;
    }
    if (ped == lastGreetingPed && !kind13 && now < lastGreetingTime + 8000) {
        return 0;
    }
    SetModel(ev, ped);
    uint32_t modelWord = ev->word0;
    lastGreetingTime = now;
    lastGreetingPed = ped;
    CVector theirs;
    CVector mine;
    CVector* otherForward = Placeable_GetForward(other, &theirs);
    CVector* pedForward = Placeable_GetForward(ped, &mine);
    float facing = DotProduct(pedForward, otherForward);
    int model = int((ev->word0 >> 9) & 0xFF);
    uint8_t* vehicle = At<uint8_t*>(other, 0x1554);
    if (vehicle != nullptr) {
        int32_t kind = At<int32_t>(vehicle, 0x46C);
        if (kind == 1) {
            ev->word0 = SetId(modelWord, 0x15);
            return 1;
        }
        if (kind != 0) {
            return 1;
        }
        if (RandomChance_41A5E0() != 0) {
            ev->word0 = SetId(ev->word0, Has(0x19, model) ? 0x19 : 0x15);
        } else {
            ev->word0 = SetId(ev->word0, Has(0x1C, model) ? 0x1C : 0x15);
        }
        return 1;
    }
    uint8_t* ownVehicle = At<uint8_t*>(ped, 0x1554);
    if (ownVehicle != nullptr && At<int32_t>(ownVehicle, 0x46C) == 1) {
        ev->word0 = SetId(modelWord, 0x15);
        return 1;
    }
    int32_t attitude;
    Ped_GetAttitudeTo(ped, &attitude, At<int32_t>(other, 0x1310));
    switch (attitude) {
    case 0:
        ev->word0 = SetId(ev->word0, 0x15);
        break;
    case 1:
        ev->word0 = SetId(ev->word0, 0x15);
        if (double(facing) < -0.800000011920929 && RandomInt(0, 100) < 0x28) {
            ev->word0 = SetId(ev->word0, 0x13);
        }
        break;
    case 2:
        if (double(facing) <= 0.800000011920929 || std::isnan(facing)) {
            uint32_t id = RandomChance_41A5E0() == 0 ? 0x15 : 0x13;
            ev->word0 ^= (id ^ ev->word0) & kIdMask;
        } else {
            ev->word0 = SetId(ev->word0, 0x15);
        }
        break;
    case 3:
        ev->word0 = SetId(ev->word0, 0x13);
        if (double(facing) > 0.800000011920929 && RandomInt(0, 100) < 0x28) {
            ev->word0 = SetId(ev->word0, 0x15);
        }
        break;
    case 4:
        ev->word0 = SetId(ev->word0, 0x13);
        break;
    default:
        break;
    }
    if ((ev->word0 & kIdMask) != 0x15 || !kind13) {
        return 1;
    }
    static const uint32_t bySeason[5] = {0x1B, 0x1D, 0x18, 0x1A, 0x16};
    uint32_t season = uint32_t(Clothes_Fn6CB8F0(playerClothes) - 3);
    if (season <= 4) {
        uint32_t id = bySeason[season];
        if (Has(int(id), int((ev->word0 >> 9) & 0xFF))) {
            ev->word0 = SetId(ev->word0, id);
        }
    }
    if (between && Has(0x17, int((ev->word0 >> 9) & 0xFF))) {
        ev->word0 = SetId(ev->word0, 0x17);
    }
    return 1;
}

// 0x5A3B80: an insult about the player's clothes (kind 13 targets), or a clique line
uint8_t __cdecl SpeechFilter_Clothes(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    ev->timeout = 0x2EE0;
    if (PedFlags_Fn48EBA0(ped + 0xB44) != 0) {
        At<uint8_t>(ev, 9) &= 0xF;
    }
    uint8_t* target = TargetPed(ped);
    ev->word0 = (ev->word0 & 0xFE1DFFFF) | 0x1C0000;
    Speech_SetPriority(TheSpeech, ped, target, ev);
    if (Population_Fn49E410(population, ped) != 0) {
        if (Rand() * 2 / 0x7FFF >= 2) {
            ev->word0 = SetId(ev->word0, 0x1B9);
            return 1;
        }
        int32_t kind = At<int32_t>(ped, 0x1310);
        uint32_t id;
        if (kind == 4) {
            id = 0x1BF;
        } else if (kind == 3) {
            id = 0x1BE;
        } else if (kind == 5) {
            id = 0x1C0;
        } else {
            return 1;
        }
        if (Has(int(id), ModelOf(ped))) {
            ev->word0 = SetId(ev->word0, id);
        }
        return 1;
    }
    if (target == nullptr) {
        return 1;
    }
    if (At<int32_t>(target, 0x1310) != 0xD) {
        ev->word0 = SetId(ev->word0, 0x1B9);
        return 1;
    }
    if (chapter + 1 == 1) {
        if (RandomInt(0, 9) == 3) {
            ev->word0 = SetId(ev->word0, Has(0x1C1, ModelOf(ped)) ? 0x1C1 : 0x1B9);
            return 1;
        }
        if (RandomChance_41A5E0() == 0) {
            ev->word0 = SetId(ev->word0, 0x1B9);
            return 1;
        }
    } else if (RandomChance_41A5E0() == 0) {
        ev->word0 = SetId(ev->word0, 0x1B9);
        return 1;
    }
    uint32_t line = uint32_t(Speech_PickClothingLine(ped, 0));
    ev->word0 ^= (ev->word0 ^ line) & kIdMask;
    return 1;
}

// 0x5A3EC0: a line at the target by its kind (1..5), sometimes 0x1D5 instead of 0x71
uint8_t __cdecl SpeechFilter_ByTargetKind(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    ev->word0 = SetId(ev->word0, 0x71);
    ev->timeout = 0x1D4C;
    uint8_t* target = TargetPed(ped);
    ev->word0 = (ev->word0 & 0xFE1FFFFF) | 0x1E0000;
    Speech_SetPriority(TheSpeech, ped, target, ev);
    if (target == nullptr) {
        return 1;
    }
    static const uint32_t byKind[5] = {0x75, 0x74, 0x72, 0x73, 0x76};
    uint32_t kind = uint32_t(At<int32_t>(target, 0x1310) - 1);
    if (kind > 4) {
        ev->word0 = SetId(ev->word0, 0x71);
    } else if (RandomChance_41A5E0() != 0 && Has(int(byKind[kind]), ModelOf(ped))) {
        ev->word0 = SetId(ev->word0, byKind[kind]);
    }
    if ((ev->word0 & kIdMask) == 0x71 && Rand() < 0x3FFF && Has(0x1D5, ModelOf(ped))) {
        ev->word0 = SetId(ev->word0, 0x1D5);
    }
    return 1;
}

// 0x5A42B0: priority 0xF, 0x1A8 for peds with +0x1398
uint8_t __cdecl SpeechFilter_Line1A8(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    uint8_t* target = TargetPed(ped);
    ev->word0 = (ev->word0 & 0xFE1FFFFF) | 0x1E0000;
    Speech_SetPriority(TheSpeech, ped, target, ev);
    if (At<int32_t>(ped, 0x1398) != 0 && Has(0x1A8, ModelOf(ped))) {
        ev->word0 = SetId(ev->word0, 0x1A8);
    }
    return 1;
}

// 0x5A4350: priority 0xF, 12 s; queued for the player, not priority-checked at him
uint8_t __cdecl SpeechFilter_PlayerFlags(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    uint8_t* target = TargetPed(ped);
    ev->timeout = 0x2EE0;
    if (PedFlags_Fn48EBA0(ped + 0xB44) != 0) {
        At<uint8_t>(ev, 9) &= 0xF;
    }
    ev->word0 = (ev->word0 & 0xFE1FFFFF) | 0x1E0000;
    Speech_SetPriority(TheSpeech, ped, target, ev);
    uint8_t* player = playerPed;
    if (ped == player) {
        At<uint8_t>(ev, 8) = uint8_t((At<uint8_t>(ev, 8) & 0xF7) | 1);
        return 1;
    }
    if (target == player) {
        At<uint8_t>(ev, 8) &= 0xF7;
    }
    return 1;
}

// 0x5A4400: priority 0xF, 1 s
uint8_t __cdecl SpeechFilter_Short15(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    ev->word0 = (ev->word0 & 0xFE1FFFFF) | 0x1E0000;
    ev->timeout = 1000;
    Speech_SetPriority(TheSpeech, ped, TargetPed(ped), ev);
    ev->word4 &= 0xFFFFF800;
    At<uint8_t>(ev, 9) &= 0xF;
    return 1;
}

// the voice (VoiceFor) as the model, priority 0xE
inline void SetVoice(SpeechEvent* ev, uint8_t* ped) {
    uint32_t voice = uint8_t(Speech_VoiceFor(ped));
    ev->word0 = ((voice | 0xE00) << 9) | (ev->word0 & 0xFE1C01FF);
}

// 0x5A4480: in the ped's voice, 1 s
uint8_t __cdecl SpeechFilter_Voice(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    SetVoice(ev, ped);
    ev->timeout = 1000;
    Speech_SetPriority(TheSpeech, ped, TargetPed(ped), ev);
    ev->word4 &= 0xFFFFF800;
    At<uint8_t>(ev, 9) &= 0xF;
    return 1;
}

// 0x5A4510: line 0x7F in the ped's voice, 1 s
uint8_t __cdecl SpeechFilter_Voice7F(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    ev->word0 = SetId(ev->word0, 0x7F);
    SetVoice(ev, ped);
    ev->timeout = 1000;
    Speech_SetPriority(TheSpeech, ped, TargetPed(ped), ev);
    ev->word4 &= 0xFFFFF800;
    At<uint8_t>(ev, 9) &= 0xF;
    return 1;
}

// 0x5A45B0: priority 3, 1 s
uint8_t __cdecl SpeechFilter_Short3(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    ev->word0 = (ev->word0 & 0xFE07FFFF) | 0x60000;
    ev->timeout = 1000;
    Speech_SetPriority(TheSpeech, ped, TargetPed(ped), ev);
    ev->word4 &= 0xFFFFF800;
    At<uint8_t>(ev, 9) &= 0xF;
    return 1;
}

// ---- the filters with no id range of their own (Speech_InitFilters' table) ------------------

// 0x5A05D0: priority 0x16, 12 s
uint8_t __cdecl SpeechFilter_P16Long(void*, void*, SpeechEvent* ev) {
    ev->word0 = (ev->word0 & 0xFE2DFFFF) | 0x2C0000;
    ev->timeout = 0x2EE0;
    return 1;
}

// 0x5A05F0: by +0x2C (1, 2, 3: a fixed model, else the speaker's), priority 0x13, 7.5 s
uint8_t __cdecl SpeechFilter_ByKind2C(uint8_t* ped, void*, SpeechEvent* ev) {
    int32_t kind = ev->unknown2C;
    if (kind == 1) {
        ev->word0 = (ev->word0 & 0xFFFEE5FF) | 0xE400;
    } else if (kind == 2) {
        ev->word0 = (ev->word0 & 0xFFFEE3FF) | 0xE200;
    } else if (kind == 3) {
        ev->word0 = (ev->word0 & 0xFFFF19FF) | 0x11800;
    } else if (ped != nullptr) {
        SetModel(ev, ped);
    }
    ev->word0 = (ev->word0 & 0xFE27FFFF) | 0x260000;
    ev->timeout = 0x1D4C;
    return 1;
}

// 0x5A0680, 0x5A06B0, 0x5A06E0, 0x5A0710: a fixed model and priority (+0xA bit 2), 7.5 or 12 s
uint8_t __cdecl SpeechFilter_Fixed0680(void*, void*, SpeechEvent* ev) {
    uint32_t word = ev->word0;
    At<uint8_t>(ev, 0xA) |= 4;
    ev->word0 = (word & 0xFE1677FF) | 0x167600;
    ev->timeout = 0x1D4C;
    return 1;
}
uint8_t __cdecl SpeechFilter_Fixed06B0(void*, void*, SpeechEvent* ev) {
    uint32_t word = ev->word0;
    At<uint8_t>(ev, 0xA) |= 4;
    ev->word0 = (word & 0xFE2277FF) | 0x227600;
    ev->timeout = 0x1D4C;
    return 1;
}
uint8_t __cdecl SpeechFilter_Fixed06E0(void*, void*, SpeechEvent* ev) {
    uint32_t word = ev->word0;
    At<uint8_t>(ev, 0xA) |= 4;
    ev->word0 = (word & 0xFE2277FF) | 0x227600;
    ev->timeout = 0x2EE0;
    return 1;
}
uint8_t __cdecl SpeechFilter_Fixed0710(void*, void*, SpeechEvent* ev) {
    uint32_t word = ev->word0;
    At<uint8_t>(ev, 0xA) |= 4;
    ev->word0 = (word & 0xFE31F7FF) | 0x31F600;
    ev->timeout = 0x1D4C;
    return 1;
}

// 0x5A0740: the model at 0xBF4030, priority 0x18, 7.5 s (+0xA bit 2)
uint8_t __cdecl SpeechFilter_Model4030(void*, void*, SpeechEvent* ev) {
    uint32_t model = *reinterpret_cast<const uint32_t*>(0x00BF4030);
    At<uint8_t>(ev, 0xA) |= 4;
    ev->timeout = 0x1D4C;
    ev->word0 ^= ((model << 9) ^ ev->word0) & 0x1FE00;
    ev->word0 = (ev->word0 & 0xFE31FFFF) | 0x300000;
    return 1;
}

// 0x5A0EF0: the speaker's model, priority 0x18, 12 s, not priority-checked; some ids turned
// into others (0x12D 0x49, 5 0x42, 0x40 0x44, 0x6E 0x45, 0x1A2 0x4A, 0x1E9 0x1EA)
uint8_t __cdecl SpeechFilter_Remap(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    At<uint8_t>(ev, 8) &= 0xF7;
    SetModel(ev, ped);
    ev->timeout = 0x2EE0;
    uint32_t word = (ev->word0 & 0xFE31FFFF) | 0x300000;
    ev->word0 = word;
    switch (word & kIdMask) {
    case 0x12D: ev->word0 = (word & 0xFFFFFE49) | 0x49; break;
    case 5: ev->word0 = (word & 0xFFFFFE42) | 0x42; break;
    case 0x40: ev->word0 = (word & 0xFFFFFE44) | 0x44; break;
    case 0x6E: ev->word0 = (word & 0xFFFFFE45) | 0x45; break;
    case 0x1A2: ev->word0 = (word & 0xFFFFFE4A) | 0x4A; break;
    case 0x1E9: ev->word0 = (word & 0xFFFFFFEA) | 0x1EA; break;
    default: break;
    }
    return 1;
}

// 0x5A0FC0: line 0x3E, one in five 0x51 / 0x5F / 0x62 when the speaker has it, priority 0x11,
// 7.5 s (a non-ped speaker reads through null for its model, as the game)
uint8_t __cdecl SpeechFilter_Line3E(uint8_t* ped, void*, SpeechEvent* ev) {
    uint8_t* speaker = ped == nullptr ? nullptr : AsPed(ped);
    int pick = Rand() * 4 / 0x7FFF;
    ev->word0 = (ev->word0 & 0xFFFFFE3E) | 0x3E;
    if (pick == 2) {
        if (Has(0x51, ModelOf(speaker))) {
            ev->word0 = (ev->word0 & 0xFFFFFE51) | 0x51;
        }
    } else if (pick == 3) {
        if (Has(0x5F, ModelOf(speaker))) {
            ev->word0 = (ev->word0 & 0xFFFFFE5F) | 0x5F;
        }
    } else if (pick == 4) {
        if (Has(0x62, ModelOf(speaker))) {
            ev->word0 = (ev->word0 & 0xFFFFFE62) | 0x62;
        }
    }
    ev->word0 = (ev->word0 & 0xFE23FFFF) | 0x220000;
    ev->timeout = 0x1D4C;
    return 1;
}

// 0x5A11B0: at most every 5 s (0xCF4430), a ped's: priority 0x16, queued, 7.5 s
uint8_t __cdecl SpeechFilter_Alert(uint8_t* ped, void*, SpeechEvent* ev) {
    uint8_t* speaker = ped == nullptr ? nullptr : AsPed(ped);
    if (lastAlertTime >= m_snTimeInMilliseconds - 5000 || speaker == nullptr) {
        return 0;
    }
    ev->word0 = (ev->word0 & 0xFE2DFFFF) | 0x2C0000;
    At<uint8_t>(ev, 8) |= 1;
    ev->timeout = 0x1D4C;
    lastAlertTime = m_snTimeInMilliseconds;
    return 1;
}

// 0x5A1220 (the default for ids 0xC1..0x117): priority 0x16, queued, 12 s; 0x16F becomes 0x13D
// when the player's weapon slot (+0x1D0) holds one of the two models, 0x150..0x156 become 0x13E
// when he holds a weapon line's
uint8_t __cdecl SpeechFilter_Default1220(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    uint32_t word = ev->word0;
    if ((word & kIdMask) == 0x16F) {
        int32_t model = At<int16_t>(At<uint8_t*>(playerPed, 0x1D0), 0x10E);
        if (model == *reinterpret_cast<const int32_t*>(0x00BF4430) || model == *reinterpret_cast<const int32_t*>(0x00BF433C)) {
            ev->word0 = (word & 0xFFFFFF3D) | 0x13D;
        }
    }
    uint32_t current = ev->word0;
    uint32_t id = current & kIdMask;
    if (id >= 0x150 && id <= 0x156 && Speech_PlayerHasWeaponLine() != 0) {
        ev->word0 = (current & 0xFFFFFF3E) | 0x13E;
    }
    ev->word0 = (ev->word0 & 0xFE2DFFFF) | 0x2C0000;
    At<uint8_t>(ev, 8) |= 1;
    ev->timeout = 0x2EE0;
    return 1;
}

// 0x5A12D0: the speaker's model, 12 s; 0xB1: priority 0x15, urgent (+8 0xA0, +0xA bit 0), else
// priority 0x16 (+9 bits 2..3), not priority-checked either way
uint8_t __cdecl SpeechFilter_Line12D0(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    SetModel(ev, ped);
    ev->timeout = 0x2EE0;
    uint32_t word = ev->word0;
    if ((word & kIdMask) == 0xB1) {
        At<uint8_t>(ev, 9) &= 0xFB;
        At<uint8_t>(ev, 0xA) |= 1;
        ev->word0 = (word & 0xFE2BFFFF) | 0x2A0000;
        uint8_t flags = uint8_t((At<uint8_t>(ev, 8) & 0xF7) | 0xA0);
        At<uint8_t>(ev, 9) &= 0xCF;
        At<uint8_t>(ev, 8) = flags;
        return 1;
    }
    At<uint8_t>(ev, 8) &= 0xF7;
    At<uint8_t>(ev, 9) |= 0xC;
    ev->word0 = (word & 0xFE2DFFFF) | 0x2C0000;
    At<uint8_t>(ev, 9) &= 0xCF;
    return 1;
}

// 0x5A1370 (the default for ids 0x0..0x61 and 0x118..0x1ED's last ones; Speech_PassesPedFlag
// lets its events through): the speaker's model, priority 0x18, 12 s, not priority-checked;
// 0x6D and 0x11A urgent (+8 0xA0), 0x11A for a minute and not queued
uint8_t __cdecl SpeechFilter_Default1370(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    At<uint8_t>(ev, 8) &= 0xF7;
    SetModel(ev, ped);
    ev->timeout = 0x2EE0;
    uint32_t word = (ev->word0 & 0xFE31FFFF) | 0x300000;
    uint8_t flags = At<uint8_t>(ev, 8);
    ev->word0 = word;
    uint32_t id = word & kIdMask;
    if (id == 0x6D || id == 0x11A) {
        At<uint8_t>(ev, 9) &= 0xF;
        At<uint8_t>(ev, 0xA) &= 0xFD;
        flags |= 0xA0;
        uint8_t flags9 = At<uint8_t>(ev, 9);
        At<uint8_t>(ev, 8) = flags;
        if (id == 0x11A) {
            flags &= 0x77;
            flags9 &= 0xFA;
            ev->timeout = 0xEA60;
            At<uint8_t>(ev, 8) = flags;
            At<uint8_t>(ev, 9) = flags9;
        }
    }
    return 1;
}

// 0x5A15C0: in the voice at 0xBF3EB0 for special-model peds, else 0xBF3F7C (+9 bits 4..7 cleared)
uint8_t __cdecl SpeechFilter_SpecialVoice(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) != nullptr && Ped_GetModelInfo(ped)[0x50] != 0) {
        uint32_t model = *reinterpret_cast<const uint32_t*>(0x00BF3EB0);
        ev->word0 ^= ((model << 9) ^ ev->word0) & 0x1FE00;
        At<uint8_t>(ev, 9) &= 0xF;
        return 1;
    }
    uint32_t model = *reinterpret_cast<const uint32_t*>(0x00BF3F7C);
    At<uint8_t>(ev, 9) &= 0xF;
    ev->word0 ^= ((model << 9) ^ ev->word0) & 0x1FE00;
    return 1;
}

// 0x5A1A00: a ped's: priority 0x16
uint8_t __cdecl SpeechFilter_P16(uint8_t* ped, void*, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    ev->word0 = (ev->word0 & 0xFE2DFFFF) | 0x2C0000;
    return 1;
}

// 0x5A2A70: a ped within 9 of the player, at most every half second (0xCF6C74): priority 0xA,
// 1 s, priority-checked at its target, no variant
uint8_t __cdecl SpeechFilter_NearPlayer(uint8_t* ped, void*, SpeechEvent* ev) {
    uint32_t now = m_snTimeInMilliseconds;
    uint8_t* speaker = ped == nullptr ? nullptr : AsPed(ped);
    if (now <= lastNoticeTime + 0x1F4 || speaker == nullptr) {
        return 0;
    }
    CVector d;
    VectorSub(&d, PositionOf(speaker), PositionOf(playerPed));
    float squared = float((double(d.x) * d.x + double(d.y) * d.y) + double(d.z) * d.z);
    float distance = crt::Sqrtf(std::fabs(squared));
    if (distance > *reinterpret_cast<const float*>(0x00900E04)) {   // (9.0)
        return 0;
    }
    ev->word0 = (ev->word0 & 0xFE15FFFF) | 0x140000;
    ev->timeout = 1000;
    Speech_SetPriority(TheSpeech, speaker, TargetPed(speaker), ev);
    ev->word4 &= 0xFFFFF800;
    lastNoticeTime = now;
    return 1;
}

// 0x5A2F90: priority 6, 7.5 s; at the player: 0x23 when the speaker has it (one in four when it
// also has 0x22)
uint8_t __cdecl SpeechFilter_AtPlayer23(uint8_t* ped, void*, SpeechEvent* ev) {
    uint8_t* speaker = ped == nullptr ? nullptr : AsPed(ped);
    uint8_t* target = TargetPed(speaker);
    if (speaker == nullptr) {
        return 0;
    }
    bool has23 = Has(0x23, ModelOf(speaker));
    bool has22 = Has(0x22, ModelOf(speaker));
    if (target != nullptr && target == playerPed && has23) {
        if (!has22 || RandomInt(1, 4) == 2) {
            ev->word0 = (ev->word0 & 0xFFFFFE23) | 0x23;
        }
    }
    ev->word0 = (ev->word0 & 0xFE0DFFFF) | 0xC0000;
    Speech_SetPriority(TheSpeech, speaker, target, ev);
    ev->timeout = 0x1D4C;
    return 1;
}

// 0x5A3090: not while the player's task (+0x668) holds; priority 0x12, 1 s; at a special-model
// target of models 0xE, 0x19, 0x26, 0x27 from a plain speaker: 0x96 (else 0x91); at others by
// the target's kind (+0x1310): 0 and 7 0x93, 8 0x92, 13 sometimes a clothing reply, else 0x91
uint8_t __cdecl SpeechFilter_AtTarget(uint8_t* ped, void*, SpeechEvent* ev) {
    if (PedTask_Fn4864E0(playerPed + 0x668) != 0) {
        return 0;
    }
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    ev->timeout = 1000;
    uint8_t* target = TargetPed(ped);
    ev->word0 = (ev->word0 & 0xFE25FFFF) | 0x240000;
    Speech_SetPriority(TheSpeech, ped, target, ev);
    if (target == nullptr) {
        return 1;
    }
    auto say = [ev](uint32_t id) { ev->word0 = SetId(ev->word0, id); };
    if (Ped_GetModelInfo(ped)[0x50] == 0 && Ped_GetModelInfo(target)[0x50] != 0) {
        uint16_t model = At<uint16_t>(target, 0x10E);
        if (model == 0xE || model == 0x19 || model == 0x26 || model == 0x27) {
            say(Has(0x96, ModelOf(ped)) ? 0x96 : 0x91);
        }
        return 1;
    }
    switch (At<int32_t>(target, 0x1310)) {
    case 0:
    case 7:
        say(Has(0x93, ModelOf(ped)) ? 0x93 : 0x91);
        return 1;
    case 8:
        Ped_GetModelInfo(target);
        say(Has(0x92, ModelOf(ped)) ? 0x92 : 0x91);
        return 1;
    case 13:
        if (RandomChance_41A5E0() != 0) {
            int id = Speech_PickClothingLine(ped, 1);
            ev->word0 ^= (ev->word0 ^ uint32_t(id)) & kIdMask;
            return 1;
        }
        say(0x91);
        return 1;
    default:
        say(0x91);
        return 1;
    }
}

// 0x5A32C0: not while the player's task (+0x668) holds; a taunt (0x5A1410) at priority 0xC, 12 s
uint8_t __cdecl SpeechFilter_TauntAtTarget(uint8_t* ped, uint8_t* target, SpeechEvent* ev) {
    if (AsPed(ped) == nullptr) {
        return 0;
    }
    if (PedTask_Fn4864E0(playerPed + 0x668) != 0) {
        return 0;
    }
    if (SpeechFilter_Taunt(ped, target, ev) == 0) {
        return 0;
    }
    ev->word0 = (ev->word0 & 0xFE19FFFF) | 0x180000;
    ev->timeout = 0x2EE0;
    Speech_SetPriority(TheSpeech, ped, TargetPed(ped), ev);
    return 1;
}

// 0x5A3DD0: a clothing line at the player (0x98 asks for the reply), priority 0x13, 7.5 s; not
// when it comes out as the none line (0x1B9, the reply's 0x91)
uint8_t __cdecl SpeechFilter_ClothingAtPlayer(uint8_t* ped, void*, SpeechEvent* ev) {
    uint8_t* speaker = ped == nullptr ? nullptr : AsPed(ped);
    uint8_t reply = (ev->word0 & kIdMask) == 0x98 ? 1 : 0;
    if (speaker == nullptr) {
        return 0;
    }
    ev->timeout = 0x1D4C;
    if (PedFlags_Fn48EBA0(speaker + 0xB44) != 0) {
        At<uint8_t>(ev, 9) &= 0xF;
    }
    if (TargetPed(speaker) != playerPed) {
        return 0;
    }
    ev->word0 = (ev->word0 & 0xFE27FFFF) | 0x260000;
    int id = Speech_PickClothingLine(speaker, reply);
    ev->word0 ^= (ev->word0 ^ uint32_t(id)) & kIdMask;
    uint32_t said = ev->word0 & kIdMask;
    if (said == (reply != 0 ? 0x91u : 0x1B9u)) {
        return 0;
    }
    return 1;
}

// 0x5A40D0: a ped that moved (from where it last spoke this, 0xCF6C88), or another ped or 10 s
// later: priority 0xF, priority-checked at its target; the player's 0x77 is 0x136
uint8_t __cdecl SpeechFilter_Moved(uint8_t* ped, void*, SpeechEvent* ev) {
    if ((lastSeenPosGuard & 1) == 0) {
        lastSeenPosGuard |= 1;
        lastSeenPos[0] = 0.0f;
        lastSeenPos[1] = 0.0f;
        lastSeenPos[2] = 0.0f;
    }
    uint32_t now = m_snTimeInMilliseconds;
    if (ped == nullptr) {
        return 0;
    }
    if (AsPed(ped) == playerPed && (ev->word0 & kIdMask) == 0x77) {
        ev->word0 = (ev->word0 & 0xFFFFFF36) | 0x136;
    }
    if (now <= lastSeenTime + 10000 && ped == lastSeenPed) {
        return 0;
    }
    const CVector* position = PositionOf(ped);
    if (VectorNotEqual(position, reinterpret_cast<const CVector*>(&lastSeenPos[0])) == 0) {
        return 0;
    }
    if ((At<uint8_t>(ped, 0x108) & 7) != 3) {
        return 0;
    }
    uint8_t* target = TargetPed(ped);
    ev->word0 = (ev->word0 & 0xFE1FFFFF) | 0x1E0000;
    Speech_SetPriority(TheSpeech, ped, target, ev);
    lastSeenPed = ped;
    lastSeenTime = now;
    std::memcpy(&lastSeenPos[0], PositionOf(ped), 0xC);
    return 1;
}

// 0x5A4210: a ped's, a second after 0xCF6C98 (which nothing sets): priority 0xF, priority-checked
// at its target, not checked (+8 bit 3) and +9 bits 4..7 cleared
uint8_t __cdecl SpeechFilter_Comment(uint8_t* ped, void*, SpeechEvent* ev) {
    if ((unusedPosGuard & 1) == 0) {
        unusedPosGuard |= 1;
        unusedPos[0] = 0.0f;
        unusedPos[1] = 0.0f;
        unusedPos[2] = 0.0f;
    }
    if (ped == nullptr || m_snTimeInMilliseconds <= lastCommentTime + 1000) {
        return 0;
    }
    if ((At<uint8_t>(ped, 0x108) & 7) != 3) {
        return 0;
    }
    uint8_t* target = TargetPed(ped);
    ev->word0 = (ev->word0 & 0xFE1FFFFF) | 0x1E0000;
    Speech_SetPriority(TheSpeech, ped, target, ev);
    At<uint8_t>(ev, 9) &= 0xF;
    At<uint8_t>(ev, 8) &= 0xF7;
    return 1;
}

// 0x6CB8F0 (thiscall, the player's clothes at 0x20C5F70; belongs with PlayerClothing.cpp): the
// slot kind its four worn pieces (+9, +0x15, +0x39, +0x45: indices 0..10) are most of: a tally,
// kinds 1, 2, 8 and 9 counted with 0, then 0 against 3..7 in turn (the later one on a tie). (A
// piece of 0xFF bumps the stack word below the tally in the game, its caller's saved esi; not
// here)
int32_t __fastcall Clothes_DominantKind(uint8_t* self, re::Edx) {
    int32_t counts[11] = {};
    static constexpr int kPieces[4] = {9, 0x15, 0x39, 0x45};
    for (int offset : kPieces) {
        int32_t kind = int8_t(self[offset]);
        if (kind >= 0 && kind < 11) {
            counts[kind]++;
        }
    }
    counts[0] += counts[1] + counts[2] + counts[8] + counts[9];
    int32_t best = counts[0] > counts[3] ? 0 : 3;
    for (int32_t kind = 4; kind <= 7; kind++) {
        if (!(counts[best] > counts[kind])) {
            best = kind;
        }
    }
    return best;
}

// ---- the speech manager ------------------------------------------------------------------------

// 0x5A4630 (thiscall): the filter table (+0x2D0, 0x1ED events): the defaults by id range,
// then the events with their own
void __fastcall Speech_InitFiltersImpl(uint8_t* self) {
    uint32_t counter = 0xFFFFFF48;
    for (int i = 0; i < 0x1ED; ++i, ++counter) {
        uintptr_t filter;
        if (counter - 0x77 <= 0x56) {
            filter = 0x005A1220;
        } else if (counter <= 0x61) {
            filter = 0x005A1370;
        } else {
            filter = 0x005A2A00;
        }
        At<uint32_t>(self, 0x2D0 + i * 4) = filter;
    }
    static const struct {
        uint16_t offset;
        uint32_t filter;
    } kFilters[] = {
        {0xA4C, 0x5A1370}, {0x480, 0x5A1370}, {0x47C, 0x5A1370}, {0x2E0, 0x5A1370}, {0x484, 0x5A1370},
        {0x90C, 0x5A1370}, {0x738, 0x5A1370}, {0x774, 0x5A06E0}, {0x778, 0x5A06E0}, {0x760, 0x5A06B0},
        {0x768, 0x5A06B0}, {0x76C, 0x5A06B0}, {0x770, 0x5A10B0}, {0x764, 0x5A0680}, {0x514, 0x5A3090},
        {0x9B4, 0x5A3B80}, {0xA00, 0x5A2DB0}, {0xA08, 0x5A2CC0}, {0x490, 0x5A3EC0}, {0x318, 0x5A35C0},
        {0x348, 0x5A45B0}, {0x4B8, 0x5A4210}, {0x4C0, 0x5A19A0}, {0xA38, 0x5A4350}, {0x4B4, 0x5A4400},
        {0x744, 0x5A4480}, {0x3BC, 0x5A42B0}, {0x468, 0x5A32C0}, {0x40C, 0x5A3370}, {0x57C, 0x5A2EA0},
        {0x564, 0x5A2C30}, {0x3C8, 0x5A2A70}, {0x358, 0x5A2F90}, {0x558, 0x5A1A00}, {0x61C, 0x5A0710},
        {0x6C8, 0x5A0740}, {0x978, 0x5A2BA0}, {0x530, 0x5A3DD0}, {0x9D8, 0x5A3DD0}, {0x4AC, 0x5A40D0},
        {0x4BC, 0x5A40D0}, {0x4C4, 0x5A40D0}, {0x494, 0x5A40D0}, {0x4B0, 0x5A4510}, {0x4CC, 0x5A4510},
        {0xA20, 0x5A4510}, {0x410, 0x5A3560}, {0x448, 0x5A3560}, {0x590, 0x5A12D0}, {0x594, 0x5A12D0},
        {0x678, 0x5A0710}, {0x6C0, 0x5A0710}, {0x368, 0x5A05F0}, {0x36C, 0x5A05F0}, {0x370, 0x5A05F0},
        {0x374, 0x5A05F0}, {0x378, 0x5A05F0}, {0x37C, 0x5A05F0}, {0x380, 0x5A05F0}, {0x384, 0x5A05F0},
        {0x388, 0x5A05F0}, {0x38C, 0x5A05F0}, {0x390, 0x5A05F0}, {0x394, 0x5A05F0}, {0x398, 0x5A05F0},
        {0x39C, 0x5A05F0}, {0x3A0, 0x5A05F0}, {0x3A4, 0x5A05F0}, {0x3A8, 0x5A05F0}, {0x3AC, 0x5A05F0},
        {0x3B0, 0x5A05F0}, {0x3B4, 0x5A05F0}, {0x3B8, 0x5A05F0}, {0x874, 0x5A05D0}, {0x84C, 0x5A05D0},
        {0x350, 0x5A0710}, {0x4D8, 0x5A15C0}, {0x2D8, 0x5A0FC0}, {0x85C, 0x5A11B0}, {0x2E4, 0x5A0EF0},
        {0x3D0, 0x5A0EF0}, {0x784, 0x5A0EF0}, {0x958, 0x5A0EF0}, {0xA74, 0x5A0EF0}, {0x488, 0x5A0EF0},
    };
    for (const auto& entry : kFilters) {
        At<uint32_t>(self, entry.offset) = entry.filter;
    }
}

// the event's defaults (the part the constructor-like code at the top of each Add repeats)
void InitEvent(SpeechEvent* ev) {
    ev->position = {0.0f, 0.0f, 0.0f};
    Memset(ev, 0, 0x30);
    ev->wordC &= ~3u;
    At<uint8_t>(ev, 0x1C) &= 0xFE;
    At<uint8_t>(ev, 0xA) = uint8_t((At<uint8_t>(ev, 0xA) & 0xFA) | 2);
    ev->timeout = 1000;
    At<uint8_t>(ev, 8) = uint8_t((At<uint8_t>(ev, 8) & 5) | 0xAA);
    At<uint8_t>(ev, 9) = 0xF8;
    ev->unknown2C = -1;
}
inline void SetBit2OfA(SpeechEvent* ev, int flag) {
    uint8_t b = At<uint8_t>(ev, 0xA);
    At<uint8_t>(ev, 0xA) = uint8_t(b ^ ((uint8_t(flag * 4) ^ b) & 4));
}

using Filter = uint8_t(__cdecl*)(void* ped, void* target, SpeechEvent* ev);

// 0x5A4920 (thiscall): builds and says an event through its filter (the id stays when
// it fails); the result also in +0xA88
SpeechEvent* __fastcall Speech_AddImpl(uint8_t* self, re::Edx, SpeechEvent* out, uint8_t* ped, void* target, int,
                                        int id, int32_t arg6, int32_t arg7, int flag8, int hasPosition,
                                        int32_t arg10, const CVector* position) {
    InitEvent(out);
    Speech_DebugEvent(ped, id);
    out->word4 &= 0xFFFFF800;
    out->unknown14[0] = uint32_t(arg6);
    out->unknown14[1] = uint32_t(arg7);
    SetBit2OfA(out, flag8);
    out->unknown2C = arg10;
    uint8_t b = At<uint8_t>(out, 0x1C);
    At<uint8_t>(out, 0x1C) = uint8_t(b ^ ((b ^ uint8_t(hasPosition)) & 1));
    out->word0 = (out->word0 & 0xFFFFFE00) | (uint32_t(id) & kIdMask);
    if (uint8_t(hasPosition) != 0 && position != nullptr) {
        out->position = *position;
    }
    if (id >= 0x1ED) {
        return out;   // (out of range)
    }
    if (ped != nullptr) {
        SetModel(out, ped);
        if ((out->word0 & 0x1FE00) == 0x7D0000) {   // (can't happen: outside the mask)
            using Fn = int(__thiscall*)(void*);
            re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0xAC / 4])(ped);
            return out;
        }
    }
    Filter filter = At<Filter>(self, 0x2D0 + id * 4);
    if (filter == nullptr) {
        return out;
    }
    if (filter(ped, target, out) != 0) {
        At<int32_t>(self, 0xA88) = Speech_Say(self, out, ped, 4);
        return out;
    }
    out->word0 = (out->word0 & 0xFFFFFE00) | (uint32_t(id) & kIdMask);
    return out;
}

// 0x5A4AD0 (thiscall): a scripted event with its variant and distance; urgent drops the
// priority checks, calm (+9) the urgency; the result in +0xA88 (-14 bad id, -15 no filter)
SpeechEvent* __fastcall Speech_AddScriptedImpl(uint8_t* self, re::Edx, SpeechEvent* out, int id, uint32_t variant,
                                                uint8_t* ped, uint32_t distance, int32_t arg6, int32_t arg7,
                                                int urgent, int flag9, int calm) {
    InitEvent(out);
    At<int32_t>(self, 0xA88) = 0;
    out->word0 ^= (out->word0 ^ uint32_t(id)) & kIdMask;
    out->unknown14[0] = uint32_t(arg6);
    SetBit2OfA(out, flag9);
    out->word4 = (out->word4 & 0xFFFFF800) | (variant & 0x7FF);
    out->unknown14[1] = uint32_t(arg7);
    if (id >= 0x1ED) {
        At<int32_t>(self, 0xA88) = -14;
        return out;
    }
    if (ped != nullptr) {
        SetModel(out, ped);
        if ((out->word0 & 0x1FE00) == 0x7D0000) {   // (can't happen: outside the mask)
            using Fn = int(__thiscall*)(void*);
            re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0xAC / 4])(ped);
            return out;
        }
    }
    Filter filter = At<Filter>(self, 0x2D0 + id * 4);
    if (filter == nullptr) {
        At<int32_t>(self, 0xA88) = -15;
        return out;
    }
    uint8_t passed = filter(ped, nullptr, out);
    out->word4 = (out->word4 & 0xFFFFF800) | (variant & 0x7FF);
    if (passed == 0) {
        return out;
    }
    if (uint8_t(calm) != 0) {
        At<uint8_t>(out, 9) &= 0xF;
    }
    if (uint8_t(urgent) != 0) {
        At<uint8_t>(out, 8) &= 0x77;
        At<uint8_t>(out, 9) &= 0xFA;
    }
    At<int32_t>(self, 0xA88) = Speech_Say(self, out, ped, distance);
    return out;
}

// 0x5A4C80 (the caller's ecx: the speech manager): the player's line 0x149
void __fastcall Speech_PlayerLine149(uint8_t* self) {
    SpeechEvent ev;
    Speech_Add(self, &ev, playerPed, nullptr, 0, 0x149, 0, 0, 0, 0, -1, nullptr);
}

// 0x5A4CB0 (thiscall): the player's line 0x14B for special models, at most every 20 s (the
// time at +0)
void __fastcall Speech_PlayerSeesModel(uint8_t* self, re::Edx, int32_t model) {
    uint32_t now = m_snTimeInMilliseconds;
    if (Speech_IsSpecialModel(model) != 0 && now > At<uint32_t>(self, 0) + 20000) {
        SpeechEvent ev;
        Speech_Add(self, &ev, playerPed, nullptr, 0, 0x14B, 0, 0, 0, 0, -1, nullptr);
        At<uint32_t>(self, 0) = now;
    }
}

// 0x5A4D10 (thiscall): the constructor
uint8_t* __fastcall Speech_Construct(uint8_t* self) {
    At<float>(self, 0xC0) = 0.0f;
    At<float>(self, 0xC4) = 0.0f;
    At<float>(self, 0xC8) = 0.0f;
    for (int i = 0; i < 16; ++i) {
        At<int32_t>(self, 0xDC + i * 0x14) = 0;
    }
    SpeechSlot* slots = reinterpret_cast<SpeechSlot*>(self + 0x214);
    for (int k = 0; k < 2; ++k) {
        SpeechSlot& slot = slots[k];
        slot.event.position = {0.0f, 0.0f, 0.0f};
        slot.ped = nullptr;
        Memset(&slot.event, 0, 0x30);
        slot.event.wordC &= ~3u;
        At<uint8_t>(&slot.event, 0x1C) &= 0xFE;
        At<uint8_t>(&slot.event, 8) = uint8_t((At<uint8_t>(&slot.event, 8) & 5) | 0xAA);
        At<uint8_t>(&slot.event, 0xA) = uint8_t((At<uint8_t>(&slot.event, 0xA) & 0xFA) | 2);
        slot.event.timeout = 1000;
        At<uint8_t>(&slot.event, 9) = 0xF8;
        slot.event.unknown2C = -1;
        slot.state = 0;
    }
    At<void*>(self, 0xA90) = nullptr;
    Speech_InitFilters(self);
    At<uint8_t>(self, 4) = 0;
    At<int32_t>(self, 0xA84) = 0;
    At<uint32_t>(self, 0xA8C) = 0xFFFFFF2F;
    for (int i = 0; i < 16; ++i) {
        uint8_t* record = self + 0xD0 + i * 0x14;
        std::memset(record, 0, 0x14);
        At<uint8_t>(record, 8) &= 0xFE;
    }
    At<uint8_t>(self, 0x210) = 0;
    At<int32_t>(self, 0x2CC) = 0;
    SetRef(self + 0xA90, nullptr);
    At<float>(self, 0xB4) = 90.0f;
    At<uint32_t>(self, 0x22C) = (At<uint32_t>(self, 0x22C) & 0xFFFFFEFF) | 0xFF;
    At<int32_t>(self, 0xA94) = 0;
    At<int32_t>(self, 0x25C) = 0;
    At<int32_t>(self, 0xB8) = 0;
    At<int32_t>(self, 0) = 0;
    At<uint8_t>(self, 0xB0) = 1;
    At<uint8_t>(self, 0xB1) = 1;
    At<uint8_t>(self, 0xB2) = 1;
    At<uint8_t>(self, 0xBC) = 0;
    return self;
}

// 0x5A4E70 (thiscall): an event with a variant: through AddScripted unless the variant is -1
SpeechEvent* __fastcall Speech_AddVariant(uint8_t* self, re::Edx, SpeechEvent* out, uint8_t* ped, int id,
                                           int32_t variant, int32_t arg5, int32_t arg6, int urgent, int flag8,
                                           int hasPosition, int32_t arg10, const CVector* position) {
    InitEvent(out);
    Speech_DebugEvent(ped, id);
    out->word4 &= 0xFFFFF800;
    out->unknown14[0] = uint32_t(arg5);
    out->word0 ^= (out->word0 ^ uint32_t(id)) & kIdMask;
    out->unknown14[1] = uint32_t(arg6);
    SetBit2OfA(out, flag8);
    out->unknown2C = arg10;
    uint8_t b = At<uint8_t>(out, 0x1C);
    At<uint8_t>(out, 0x1C) = uint8_t(b ^ ((b ^ uint8_t(hasPosition)) & 1));
    if (uint8_t(hasPosition) != 0 && position != nullptr) {
        out->position = *position;
    }
    if (id >= 0x1ED) {
        return out;
    }
    if (variant != -1) {
        SpeechEvent scripted;
        *out = *Speech_AddScripted(self, &scripted, id, uint32_t(variant), ped, 4, arg5, arg6, urgent, flag8, 0);
        return out;
    }
    if (ped != nullptr) {
        SetModel(out, ped);
        if ((out->word0 & 0x1FE00) == 0x7D0000) {   // (can't happen: outside the mask)
            using Fn = int(__thiscall*)(void*);
            re::PtrCast<Fn>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0xAC / 4])(ped);
            return out;
        }
    }
    Filter filter = At<Filter>(self, 0x2D0 + id * 4);
    if (filter == nullptr) {
        return out;
    }
    if (filter(ped, nullptr, out) != 0) {
        if (uint8_t(urgent) != 0) {
            At<uint8_t>(out, 8) &= 0x77;
            At<uint8_t>(out, 9) &= 0xFA;
        }
        At<int32_t>(self, 0xA88) = Speech_Say(self, out, ped, 4);
    }
    return out;
}

// ---- the slots ---------------------------------------------------------------------------------

SpeechSlot* OtherSlot(SpeechSlot* slot) {
    int32_t current = speechCurrentSlot;
    SpeechSlot* other = &speechSlots[current];
    if (other == slot) {
        other = &speechSlots[(current - 1) & 1];
    }
    return other;
}

// the positional volume (3): the line of sight from the camera to the speaker's head
// lowers it when blocked (+0x22 & 2 tests it)
void PlacePositional(SpeechSlot* slot, void* channel) {
    alignas(4) uint8_t colPoint[0x28];
    ColPoint_Construct(colPoint);
    CVector camera;
    GetCameraPos(&camera);
    CVector head = *PositionOf(static_cast<uint8_t*>(slot->ped));
    head.z = float(head.z + 0.8999999761581421);
    camera.z = float(0.8999999761581421 + camera.z);
    uint8_t blocked = 0;
    if ((At<uint8_t>(slot, 0x22) & 2) != 0) {
        re::Ptr32<void> hit;
        blocked = ProcessLineOfSight(&camera, &head, colPoint, &hit, 0x41, 0, 0);
    }
    CVector position = *PositionOf(static_cast<uint8_t*>(slot->ped));
    Channel_SetPosition(channel, &position, SignExtend7(slot->distance));
    float volume = 1.0f;
    if (At<int32_t>(slot->ped, 0xA4) != 0) {
        volume = 0.6499999761581421f;
    }
    if (blocked != 0) {
        float height = std::fabs(float(head.z - camera.z));
        if (height > 4.0f) {
            volume = float((0.5 - double(0.5f)) * volume);
        } else {
            float lower = float(double(height) * 0.5 * 0.25);
            volume = float((0.5 - double(lower)) * volume);
        }
    }
    Channel_SetVolume(channel, volume, -1);
    ColPoint_Destruct(colPoint);
}

// 0x5A5060 (thiscall, a slot): to a new state (the loop: some states pass on to another)
void __fastcall SpeechSlot_SetStateImpl(SpeechSlot* self, re::Edx, int32_t state) {
    int32_t index = int32_t(reinterpret_cast<uintptr_t>(self) - 0x00CF458C) / 0x5C;
    for (;;) {
        if (self->state == 3) {
            SetRef(lastSpeakerRef, nullptr);
        }
        if (uint32_t(state) > 4) {
            break;
        }
        void* channel = streamChannelList[index];
        if (state == 0 || state == 4) {
            if (speechDelayStart != 0 && state == 0 && (At<uint8_t>(self, 0x22) & 4) == 0) {
                At<uint8_t>(self, 0x22) |= 4;
                self->state = 0;
                self->history = nullptr;
                self->historyId = 0;
                self->timeout = m_snTimeInMilliseconds + 3000;
                self->subtitle = -1;
                state = 1;
                continue;
            }
            if (self->state == 0) {
                if (Channel_IsIdle(channel) != 0 || *static_cast<int32_t*>(channel) == 6) {
                    Channel_Reset(channel);
                }
                break;
            }
            using DoneCallback = uint8_t(__cdecl*)(void*, void*);
            DoneCallback callback = At<DoneCallback>(self, 0x2C);
            if (callback != nullptr) {
                void* data = At<void*>(self, 0x30);
                void* speaker = self->ped;
                At<DoneCallback>(self, 0x2C) = nullptr;
                callback(EntityAsPed(speaker), data);
            }
            int32_t historyId = self->historyId;
            if (historyId != 0) {
                void* speaker = self->ped;
                if (speaker != nullptr) {
                    uint8_t* ped = EntityAsPed(speaker);
                    if (ped != nullptr && At<int32_t>(ped, 0x14DC) == 0) {
                        History_Fn68E3C0(speechHistory, historyId, speaker);
                        Ped_Fn68EBB0(ped, 0);
                    }
                } else {
                    History_Fn68DE60(speechHistory, historyId);
                }
                self->historyId = 0;
            }
            int32_t subtitle = self->subtitle;
            if (subtitle != -1 && speechSubtitles != 0) {
                SpeechSlot* other = OtherSlot(self);
                if (other->state != 0 && other->subtitle == subtitle) {
                    self->subtitle = -1;
                    History_Fn68DDD0(speechHistory, index);
                }
            }
            if (self->subtitle != -1) {
                History_Fn68DCD0(speechHistory, index);
                self->subtitle = -1;
            }
            if (Channel_IsIdle(channel) == 0) {
                Channel_Stop(channel, 500);
            }
            if (*static_cast<int32_t*>(channel) == 6) {
                Channel_Reset(channel);
            }
            if (state != 0) {
                break;
            }
            if (self->ped != nullptr) {
                Fn5B25E0(self->ped, int(self->event.word0 & kIdMask), int(self->event.word4 & 0x7FF), 0);
            }
            uint8_t* player = playerPed;
            if (player != nullptr && player == self->ped) {
                At<int32_t>(player, 0x1440) = 0;
            }
            SetRef(&self->ped, nullptr);
            break;
        }
        if (state == 1) {
            uint8_t* player = playerPed;
            if (player != nullptr && player == self->ped) {
                At<int32_t>(player, 0x1440) = 1;
            }
            if (self->historyId != 0) {
                History_Fn68DE60(speechHistory, self->historyId);
                self->historyId = 0;
            }
            if ((At<uint8_t>(self, 0x20) & 2) == 0) {
                self->subtitle = -1;
                break;
            }
            uint32_t id = self->event.word0 & kIdMask;
            if (speechInMemory[id] == 0 && Speech_IsStreamLoaded(int(id)) == 0) {
                RequestModel(int((self->event.word0 & kIdMask) + 0x5B54), 0xD);
            } else if (speechInMemory[self->event.word0 & kIdMask] != 0 && speechSubtitles != 0) {
                History_Fn68DDA0(speechHistory, index);
            }
            if (speechSubtitles == 0) {
                SpeechSlot* second = &speechSlots[(speechCurrentSlot - 1) & 1];
                if (second->state != 0 && second->subtitle == int32_t(self->event.word0 & kIdMask)) {
                    second->subtitle = -1;
                    History_Fn68DCA0(speechHistory);
                }
            }
            self->subtitle = int32_t(self->event.word0 & kIdMask);
            break;
        }
        if (state == 2) {
            if (SpeechSlot_Preload(self, self->line, self->volume, self->ped) == 0) {
                break;   // (a debug print)
            }
            uint8_t* chosen = static_cast<uint8_t*>(Speech_ChannelOf(TheSpeech, self));
            if (chosen + 0x54 != nullptr) {
                At<uint8_t>(chosen, 0x91) = 1;
            }
            break;
        }
        // 3: playing
        if (speechSubtitles != 0) {
            SpeechSlot* other = OtherSlot(self);
            if (other->state != 0 && other->ped == self->ped && SpeechEvent_IsCalm(&other->event) == 0 &&
                SpeechEvent_IsCalm(&self->event) == 0) {
                SpeechSlot_SetState(other, 4);
            }
        } else {
            int32_t current = speechCurrentSlot;
            if (&speechSlots[current] == self) {
                SpeechSlot* other = &speechSlots[(current - 1) & 1];
                if (other->state != 0) {
                    SpeechSlot_SetState(other, 4);
                }
            }
        }
        SetRef(lastSpeakerRef, self->ped);
        void* speaker = self->ped;
        if (speaker != nullptr && (At<uint8_t>(self, 0x22) & 4) == 0) {
            PlacePositional(self, channel);
        } else if ((At<uint8_t>(self, 0x34) & 1) != 0) {
            Channel_SetPosition(channel, &self->event.position, SignExtend7(self->distance));
        } else if (speaker != nullptr || (At<uint8_t>(self, 0x22) & 4) != 0) {
            Channel_SetNoPosition(channel, 0, 1);
        } else {
            state = 4;
            continue;
        }
        int32_t handle = At<int32_t>(channel, 0x5C);
        if (handle != -1) {
            Sound_SetVolume5AF6B0(handle, 1.0f);
        }
        Channel_Play(channel);
        uint8_t* ped = EntityAsPed(self->ped);
        if (ped != nullptr && At<int32_t>(ped, 0x14DC) == 0 && Entity_GetIsOnScreen(ped) != 0) {
            Ped_Fn68EBB0(ped, 1);
            self->historyId = History_Fn68E250(speechHistory, index, self->line, self->ped);
        }
        self->timeout = m_snTimeInMilliseconds + 200;
        uint8_t* line = History_Fn68DD30(speechHistory, index, self->line, -1, -1);
        int duration = 0;
        if (line != nullptr) {
            duration = int(double(At<float>(line, 4)) * 1000.0);
        }
        Speech_ShowSubtitle(int(self->event.word0 & kIdMask), self->line, duration);
        break;
    }
    self->state = state;
}

// 0x5A57A0 (thiscall, a slot): the per-frame update
void __fastcall SpeechSlot_UpdateImpl(SpeechSlot* self) {
    switch (self->state) {
    case 1: {
        uint32_t id = self->event.word0 & kIdMask;
        if (speechInMemory[id] != 0) {
            Speech_RecordHistory(TheSpeech, int32_t(id));
            if (Channel_IsIdle(Speech_ChannelOf(TheSpeech, self)) != 0) {
                SpeechSlot_SetState(self, 2);
                return;
            }
            Channel_Stop(Speech_ChannelOf(TheSpeech, self), 200);
        } else {
            RequestModel(int(id + 0x5B54), 0xD);
        }
        if (self->timeout < m_snTimeInMilliseconds) {
            SpeechSlot_SetState(self, 0);
        }
        return;
    }
    case 2: {
        void* channel = Speech_ChannelOf(TheSpeech, self);
        if (speechMuted == 0 && Channel_Fn5A6DA0(channel) != 0) {
            using DoneCallback = uint8_t(__cdecl*)(void*, void*);
            DoneCallback callback = At<DoneCallback>(self, 0x2C);
            if (callback != nullptr) {
                if (callback(EntityAsPed(self->ped), At<void*>(self, 0x30)) == 0) {
                    return;
                }
                At<DoneCallback>(self, 0x2C) = nullptr;
                if (self->state != 2) {
                    return;
                }
            }
            SpeechSlot_SetState(self, 3);
            return;
        }
        if (self->timeout < m_snTimeInMilliseconds) {
            SpeechSlot_SetState(self, 0);
        }
        return;
    }
    case 3: {
        void* channel = Speech_ChannelOf(TheSpeech, self);
        if (Channel_IsIdle(channel) != 0 && m_snTimeInMilliseconds > self->timeout) {
            SpeechSlot_SetState(self, 4);
            return;
        }
        uint8_t* ped = static_cast<uint8_t*>(self->ped);
        if (ped == nullptr || (At<uint8_t>(self, 0x22) & 4) != 0) {
            return;
        }
        CVector position = *PositionOf(ped);
        Channel_SetPosition(channel, &position, SignExtend7(self->distance));
        float volume = 1.0f;
        if (At<int32_t>(self->ped, 0xA4) != 0) {
            volume = 0.6499999761581421f;
        }
        Channel_SetVolume(channel, volume, -1);
        return;
    }
    case 4:
        if (Channel_IsIdle(Speech_ChannelOf(TheSpeech, self)) != 0) {
            SpeechSlot_SetState(self, 0);
        }
        return;
    default:
        return;
    }
}

// 0x5A59D0 (thiscall): stops the ped's lines up to a priority (reason 1, 2: only those
// with event flag +9 & 0x10, & 0x40; 3, 4: those not playing yet), and the library's
void __fastcall Speech_StopPed(uint8_t* self, re::Edx, uint8_t* ped, int32_t reason, uint32_t priority) {
    SpeechSlot* slots = reinterpret_cast<SpeechSlot*>(self + 0x214);
    for (int k = 0; k < 2; ++k) {
        SpeechSlot* slot = &slots[k];
        int32_t state = slot->state;
        if (state == 0 || slot->ped == nullptr || slot->ped != ped || PriorityOf(slot->event.word0) > priority) {
            continue;
        }
        bool stop;
        switch (reason) {
        case 1: stop = ((At<uint8_t>(slot, 0x21) >> 4) & 1) != 0; break;
        case 2: stop = ((At<uint8_t>(slot, 0x21) >> 6) & 1) != 0; break;
        case 3:
        case 4: stop = state != 3; break;
        default: stop = true; break;
        }
        if (stop) {
            SpeechSlot_SetState(slot, 0);
        }
    }
    if (reason == 4) {
        return;
    }
    float level = float(int32_t(priority));
    float limit = 23.0f;
    float capped = (level >= limit || std::isnan(level)) ? limit : level;
    SpeechLib_Stop59F8D0(TheSpeechLib, ModelOf(ped), reason, int(capped));
}

// 0x5A5AB0 (thiscall): stops the ped's event (variant -1 any), and the library's
void __fastcall Speech_StopPedEvent(uint8_t* self, re::Edx, uint8_t* ped, uint32_t id, int32_t variant) {
    SpeechSlot* slots = reinterpret_cast<SpeechSlot*>(self + 0x214);
    for (int k = 0; k < 2; ++k) {
        SpeechSlot* slot = &slots[k];
        if (slot->state != 0 && slot->ped == ped && (slot->event.word0 & kIdMask) == id &&
            (variant == -1 || int32_t(slot->event.word4 & 0x7FF) == variant)) {
            SpeechSlot_SetState(slot, 0);
        }
    }
    SpeechLib_Stop59F960(TheSpeechLib, ModelOf(ped), int(id), variant);
}

// 0x5A5B30 (thiscall): the per-frame update: both slots, then a pending location line
// (+0xBC, a second after +0xCC): 0x169 in one box, 0x15F in the quiet zones
void __fastcall Speech_Update(uint8_t* self) {
    SpeechSlot* slots = reinterpret_cast<SpeechSlot*>(self + 0x214);
    if (speechBlockedA != 0 || streamsBlocked != 0) {
        SpeechSlot* current = &slots[At<int32_t>(self, 0x2CC)];
        if (current->state != 0) {
            SpeechSlot_SetState(current, 0);
        }
        SpeechSlot* other = &slots[(At<int32_t>(self, 0x2CC) - 1) & 1];
        if (other->state != 0) {
            SpeechSlot_SetState(other, 0);
        }
        return;
    }
    SpeechSlot_Update(&slots[(At<int32_t>(self, 0x2CC) - 1) & 1]);
    SpeechSlot_Update(&slots[At<int32_t>(self, 0x2CC)]);
    Fn619A50();
    if (At<uint8_t>(self, 0xBC) == 0 || m_snTimeInMilliseconds <= At<uint32_t>(self, 0xCC) + 1000) {
        return;
    }
    float x = At<float>(self, 0xC0);
    float y = At<float>(self, 0xC4);
    float z = At<float>(self, 0xC8);
    SpeechEvent ev;
    if (x > -640.0f && x < -620.0f && z > 6.0f && y > -300.0f && y < -295.0f) {
        Speech_Add(self, &ev, playerPed, nullptr, 0, 0x169, 0, 0, 0, 0, -1, nullptr);
        At<uint8_t>(self, 0xBC) = 0;
        return;
    }
    if (Speech_IsListenerInQuietZone() != 0) {
        Speech_Add(self, &ev, playerPed, nullptr, 0, 0x15F, 0, 0, 0, 0, -1, nullptr);
    }
    At<uint8_t>(self, 0xBC) = 0;
}

// 0x5A5D10 (thiscall, a slot): starts an event from a record (0x14 bytes: volume, distance,
// flag, speaker, +0x10)
void __fastcall SpeechSlot_Start(SpeechSlot* self, re::Edx, const SpeechEvent* ev, uint8_t* record, int32_t line) {
    SpeechSlot_SetState(self, 0);
    self->volume = At<float>(record, 0);
    self->distance ^= (uint32_t(SignExtend7(At<uint32_t>(record, 4))) ^ self->distance) & 0x7F;
    uint8_t b = At<uint8_t>(self, 0xC);
    At<uint8_t>(self, 0xC) = uint8_t(b ^ ((b ^ At<uint8_t>(record, 8)) & 1));
    SetRef(&self->ped, At<void*>(record, 0xC));
    self->unknown14 = At<uint32_t>(record, 0x10);
    At<uint8_t>(record, 8) &= 0xFE;
    self->event = *ev;
    self->line = line;
    self->history = nullptr;
    self->historyId = 0;
    self->timeout = m_snTimeInMilliseconds + 3000;
    self->subtitle = -1;
    SpeechSlot_SetState(self, 1);
}

// 0x5A5DB0: stops everything
void __cdecl Speech_StopAll() {
    SpeechLib_Fn59F8A0(TheSpeechLib);
    SpeechSlot_SetState(&speechSlots[speechCurrentSlot], 0);
    SpeechSlot_SetState(&speechSlots[(speechCurrentSlot - 1) & 1], 0);
}

// ---- the speech library's callbacks (Speech::LoadDatabase sets them) -------------------------

// 0x5A0A40 (+0x4AC): a number from low up to high (rand's 0..0x7FFF scaled, truncated)
uint32_t __cdecl SpeechCallback_Random(int32_t low, int32_t high) {
    int32_t scaled = int32_t(uint32_t(Rand()) * uint32_t(high - low));
    return uint32_t(scaled / 0x7FFF + low);
}

// 0x5A0D80 (+0x4A8): the database read (LoadFile, its mode "r" whatever the library passes)
int32_t __cdecl SpeechCallback_Load(const char* path, void* buffer, int32_t size, int32_t) {
    return LoadFile_42D180(path, buffer, size, reinterpret_cast<const char*>(0x008FF548));
}

// 0x5A25A0 (+0x4B0): the time stamp
uint32_t __cdecl SpeechCallback_Now() {
    return m_snTimeInMilliseconds;
}

// 0x5A5CD0 (+0x4B8): both slots stopped (the current one first)
void __cdecl SpeechCallback_StopSlots() {
    SpeechSlot_SetState(&speechSlots[speechCurrentSlot], 0);
    SpeechSlot_SetState(&speechSlots[(speechCurrentSlot - 1) & 1], 0);
}

// 0x5A5DF0 (+0x4B4): an event started in a slot. With subtitles: an idle current slot (moving to
// the other when both are idle and its channel isn't), else the other when idle, else the first
// it can interrupt (the current, then the other) or the current when the other's event ranks at
// least as high (each emptied first). Without: an idle current slot; an event without bit 0 of
// +8 replaces the current one; the other idle: the current one finishing (unless playing) and
// the other taken; both busy: by the priorities (bits 17..24), 1 when it outranks neither
static int32_t SpeechStartIn(int32_t slot, SpeechEvent* ev, uint8_t* record, int32_t line, bool empty) {
    if (empty) {
        SpeechSlot_SetState(&speechSlots[slot], 0);
    }
    SpeechSlot_Start(&speechSlots[speechCurrentSlot], re::Edx{}, ev, record, line);
    return 0;
}
int32_t __cdecl SpeechCallback_Start(SpeechEvent* ev, void* lineArg, const char*, int32_t recordArg) {
    int32_t line = int32_t(reinterpret_cast<intptr_t>(lineArg));
    auto* record = reinterpret_cast<uint8_t*>(intptr_t(recordArg));
    int32_t cur = speechCurrentSlot;
    int32_t other = (cur - 1) & 1;
    SpeechSlot* current = &speechSlots[cur];
    SpeechSlot* second = &speechSlots[other];
    if (speechSubtitles != 0) {
        if (current->state == 0) {
            if (second->state == 0 && Channel_IsIdle(streamChannelList[cur]) == 0) {
                speechCurrentSlot = other;
            }
            return SpeechStartIn(cur, ev, record, line, false);
        }
        if (second->state == 0) {
            speechCurrentSlot = other;
            return SpeechStartIn(other, ev, record, line, false);
        }
        if (current->state != 3 && Speech_CanInterrupt2(TheSpeech, current, ev) != 0) {
            return SpeechStartIn(cur, ev, record, line, true);
        }
        if (second->state != 3 && Speech_CanInterrupt2(TheSpeech, second, ev) != 0) {
            speechCurrentSlot = other;
            return SpeechStartIn(other, ev, record, line, true);
        }
        if (SpeechEvent_Compare2(TheSpeech, &second->event, &current->event) >= 0) {
            return SpeechStartIn(cur, ev, record, line, true);
        }
        speechCurrentSlot = other;
        return SpeechStartIn(other, ev, record, line, true);
    }
    if (current->state == 0) {
        return SpeechStartIn(cur, ev, record, line, false);
    }
    if ((ev->flags8 & 1) == 0) {
        return SpeechStartIn(cur, ev, record, line, true);
    }
    if (second->state == 0) {
        if (current->state != 3) {
            SpeechSlot_SetState(current, 4);
        }
        speechCurrentSlot = (speechCurrentSlot - 1) & 1;
        return SpeechStartIn(speechCurrentSlot, ev, record, line, false);
    }
    auto priority = [](uint32_t word0) { return (word0 >> 17) & 0xFF; };
    uint32_t mine = priority(ev->word0);
    if (mine > priority(current->event.word0) && (current->state != 3 || mine <= priority(second->event.word0))) {
        return SpeechStartIn(cur, ev, record, line, true);
    }
    if (mine <= priority(second->event.word0)) {
        return 1;
    }
    speechCurrentSlot = other;
    return SpeechStartIn(other, ev, record, line, true);
}

// ---- registration ---------------------------------------------------------------------------

#define SPEECHMGR2_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(speechmgr2_original_, __LINE__);                                      \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(speechmgr2_original_, __LINE__), \
             re::Mode::Original)

SPEECHMGR2_REVERSED("SpeechFilter::P16Long", 0x005A05D0, &SpeechFilter_P16Long);
SPEECHMGR2_REVERSED("SpeechFilter::ByKind2C", 0x005A05F0, &SpeechFilter_ByKind2C);
SPEECHMGR2_REVERSED("SpeechFilter::Fixed0680", 0x005A0680, &SpeechFilter_Fixed0680);
SPEECHMGR2_REVERSED("SpeechFilter::Fixed06B0", 0x005A06B0, &SpeechFilter_Fixed06B0);
SPEECHMGR2_REVERSED("SpeechFilter::Fixed06E0", 0x005A06E0, &SpeechFilter_Fixed06E0);
SPEECHMGR2_REVERSED("SpeechFilter::Fixed0710", 0x005A0710, &SpeechFilter_Fixed0710);
SPEECHMGR2_REVERSED("SpeechFilter::Model4030", 0x005A0740, &SpeechFilter_Model4030);
SPEECHMGR2_REVERSED("SpeechFilter::Remap", 0x005A0EF0, &SpeechFilter_Remap);
SPEECHMGR2_REVERSED("SpeechFilter::Line3E", 0x005A0FC0, &SpeechFilter_Line3E);
SPEECHMGR2_REVERSED("SpeechFilter::Alert", 0x005A11B0, &SpeechFilter_Alert);
SPEECHMGR2_REVERSED("SpeechFilter::Default1220", 0x005A1220, &SpeechFilter_Default1220);
SPEECHMGR2_REVERSED("SpeechFilter::Line12D0", 0x005A12D0, &SpeechFilter_Line12D0);
SPEECHMGR2_REVERSED("SpeechFilter::Default1370", 0x005A1370, &SpeechFilter_Default1370);
SPEECHMGR2_REVERSED("SpeechFilter::SpecialVoice", 0x005A15C0, &SpeechFilter_SpecialVoice);
SPEECHMGR2_REVERSED("SpeechFilter::P16", 0x005A1A00, &SpeechFilter_P16);
SPEECHMGR2_REVERSED("SpeechFilter::NearPlayer", 0x005A2A70, &SpeechFilter_NearPlayer);
SPEECHMGR2_REVERSED("SpeechFilter::AtPlayer23", 0x005A2F90, &SpeechFilter_AtPlayer23);
SPEECHMGR2_REVERSED("SpeechFilter::AtTarget", 0x005A3090, &SpeechFilter_AtTarget);
SPEECHMGR2_REVERSED("SpeechFilter::TauntAtTarget", 0x005A32C0, &SpeechFilter_TauntAtTarget);
SPEECHMGR2_REVERSED("SpeechFilter::ClothingAtPlayer", 0x005A3DD0, &SpeechFilter_ClothingAtPlayer);
SPEECHMGR2_REVERSED("SpeechFilter::Moved", 0x005A40D0, &SpeechFilter_Moved);
SPEECHMGR2_REVERSED("SpeechFilter::Comment", 0x005A4210, &SpeechFilter_Comment);
SPEECHMGR2_REVERSED("CPlayerClothing::DominantKind", 0x006CB8F0, &Clothes_DominantKind);
SPEECHMGR2_REVERSED("SpeechFilter::Reply", 0x005A3370, &SpeechFilter_Reply);
SPEECHMGR2_REVERSED("SpeechFilter::Line5E", 0x005A3560, &SpeechFilter_Line5E);
SPEECHMGR2_REVERSED("SpeechFilter::Greeting", 0x005A35C0, &SpeechFilter_Greeting);
SPEECHMGR2_REVERSED("SpeechFilter::Clothes", 0x005A3B80, &SpeechFilter_Clothes);
SPEECHMGR2_REVERSED("SpeechFilter::ByTargetKind", 0x005A3EC0, &SpeechFilter_ByTargetKind);
SPEECHMGR2_REVERSED("SpeechFilter::Line1A8", 0x005A42B0, &SpeechFilter_Line1A8);
SPEECHMGR2_REVERSED("SpeechFilter::PlayerFlags", 0x005A4350, &SpeechFilter_PlayerFlags);
SPEECHMGR2_REVERSED("SpeechFilter::Short15", 0x005A4400, &SpeechFilter_Short15);
SPEECHMGR2_REVERSED("SpeechFilter::Voice", 0x005A4480, &SpeechFilter_Voice);
SPEECHMGR2_REVERSED("SpeechFilter::Voice7F", 0x005A4510, &SpeechFilter_Voice7F);
SPEECHMGR2_REVERSED("SpeechFilter::Short3", 0x005A45B0, &SpeechFilter_Short3);
SPEECHMGR2_REVERSED("Speech::InitFilters", 0x005A4630, &Speech_InitFiltersImpl);
SPEECHMGR2_REVERSED("Speech::Add", 0x005A4920, &Speech_AddImpl);
SPEECHMGR2_REVERSED("Speech::AddScripted", 0x005A4AD0, &Speech_AddScriptedImpl);
SPEECHMGR2_REVERSED("Speech::PlayerLine149", 0x005A4C80, &Speech_PlayerLine149);
SPEECHMGR2_REVERSED("Speech::PlayerSeesModel", 0x005A4CB0, &Speech_PlayerSeesModel);
SPEECHMGR2_REVERSED("Speech::Speech", 0x005A4D10, &Speech_Construct);
SPEECHMGR2_REVERSED("Speech::AddVariant", 0x005A4E70, &Speech_AddVariant);
SPEECHMGR2_REVERSED("SpeechSlot::SetState", 0x005A5060, &SpeechSlot_SetStateImpl);
SPEECHMGR2_REVERSED("SpeechSlot::Update", 0x005A57A0, &SpeechSlot_UpdateImpl);
SPEECHMGR2_REVERSED("Speech::StopPed", 0x005A59D0, &Speech_StopPed);
SPEECHMGR2_REVERSED("Speech::StopPedEvent", 0x005A5AB0, &Speech_StopPedEvent);
SPEECHMGR2_REVERSED("Speech::Update", 0x005A5B30, &Speech_Update);
SPEECHMGR2_REVERSED("SpeechSlot::Start", 0x005A5D10, &SpeechSlot_Start);
SPEECHMGR2_REVERSED("SpeechCallback::Random", 0x005A0A40, &SpeechCallback_Random);
SPEECHMGR2_REVERSED("SpeechCallback::Load", 0x005A0D80, &SpeechCallback_Load);
SPEECHMGR2_REVERSED("SpeechCallback::Now", 0x005A25A0, &SpeechCallback_Now);
SPEECHMGR2_REVERSED("SpeechCallback::StopSlots", 0x005A5CD0, &SpeechCallback_StopSlots);
SPEECHMGR2_REVERSED("SpeechCallback::Start", 0x005A5DF0, &SpeechCallback_Start);
SPEECHMGR2_REVERSED("Speech::StopAll", 0x005A5DB0, &Speech_StopAll);

} // namespace
