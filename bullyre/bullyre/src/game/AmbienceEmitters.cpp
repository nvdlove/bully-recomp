// The ambience emitters (0x586820..0x588030): the audio's positional sound emitters
// (TheSounds+0x4D8, pointer at 0xCF4278) fed from the area's "SP_<area>" route points,
// the mission's emitters and trigger-area loops read from audio\config\trigemit.txt;
// and the ambient stream's emitter (0xAE5F60): the closest streamed ambience.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game/Pool.h"
#include "reversed.h"

#include "game/Crt.h"
namespace {

struct CVector {
    float x, y, z;
};

// a route point (0x30 bytes; the SP_ routes' points, and the mission's emitters)
struct RoutePoint {
    CVector pos;          // +0
    int32_t missionId;    // +0xC (-1 any, -2 when no mission runs, -1000000 bad)
    uint32_t hash;        // +0x10 the emitter's id
    int16_t soundIndex;   // +0x14 (the stream emitter reads +0x14 as a dword)
    int16_t bank;         // +0x16
    uint8_t unknown18[10];
    int8_t hourStart;     // +0x22
    int8_t hourEnd;       // +0x23
    int8_t type;          // +0x24 an entry of the emitter types (its range)
    int8_t volume;        // +0x25 percent
    uint8_t unknown26;
    uint8_t condition;    // +0x27
    uint8_t u28;          // +0x28
    uint8_t u29;          // +0x29
    uint8_t flags;        // +0x2A 1, 2 streamed, 4 not while its mission runs, 8 quieter while it
                          // runs, 0x10 disabled
    uint8_t unknown2B;
    int32_t soundId;      // +0x2C (the mission's emitters; 0: free)
};
static_assert(offsetof(RoutePoint, hourStart) == 0x22);
static_assert(offsetof(RoutePoint, soundId) == 0x2C);
static_assert(sizeof(RoutePoint) == 0x30);

// an emitter (0x38 bytes)
struct AmbientEmitter {
    int32_t state;        // +0 0 free, 1 loading its bank, 2 ready, 3 playing
    re::Ptr32<void> sound;          // +4 (a sound pointer: set with Sound_Set)
    CVector pos;          // +8 (666.666 when free)
    int32_t unknown14;    // +0x14
    uint8_t hourStart;    // +0x18
    uint8_t hourEnd;      // +0x19
    uint8_t condition;    // +0x1A
    uint8_t flag1B;       // +0x1B
    re::Ptr32<void> entity;         // +0x1C (a reference: set with SetRef)
    int32_t soundId;      // +0x20
    int16_t index;        // +0x24 its slot (-1 none)
    int16_t bank;         // +0x26 (-1 none)
    float range;          // +0x28
    int32_t rangeType;    // +0x2C
    int32_t id;           // +0x30
    int32_t missionId;    // +0x34
};
static_assert(offsetof(AmbientEmitter, entity) == 0x1C);
static_assert(sizeof(AmbientEmitter) == 0x38);

// a trigger-area loop's slot (0x18 bytes)
struct EmitterSlot {
    re::Ptr32<void> sound;          // +0
    uint32_t hash;        // +4 the trigger's
    int32_t soundId;      // +8
    uint8_t condition;    // +0xC
    uint8_t padD[3];
    float range;          // +0x10
    int32_t index;        // +0x14 (-1 free)
};
static_assert(sizeof(EmitterSlot) == 0x18);

// trigemit.txt's entries (0x10 bytes)
struct TriggerEmitter {
    int32_t soundId;      // +0
    uint32_t hash;        // +4 the trigger's name
    int32_t area;         // +8
    uint8_t condition;    // +0xC
    uint8_t padD[3];
};

struct AmbienceEmitters {
    re::Ptr32<AmbientEmitter> emitters[25];    // +0
    re::Ptr32<EmitterSlot> slots[10];          // +0x64
    int32_t freeEmitterHint;         // +0x8C
    int32_t numEmitters;             // +0x90
    int32_t emitterSize;             // +0x94
    int32_t freeSlotHint;            // +0x98
    int32_t numSlots;                // +0x9C
    int32_t slotSize;                // +0xA0
    int32_t unknownA4;               // +0xA4
    uint8_t disabled;                // +0xA8
    uint8_t unknownA9[0x803];
    RoutePoint missionEmitters[16];  // +0x8AC
    int32_t routeIndex;              // +0xBAC
    re::Ptr32<void> route;                     // +0xBB0
    int32_t frame;                   // +0xBB4 (0..10)
    TriggerEmitter triggers[130];    // +0xBB8
    int32_t numTriggers;             // +0x13D8
};
static_assert(offsetof(AmbienceEmitters, slots) == 0x64);
static_assert(offsetof(AmbienceEmitters, disabled) == 0xA8);
static_assert(offsetof(AmbienceEmitters, missionEmitters) == 0x8AC);
static_assert(offsetof(AmbienceEmitters, triggers) == 0xBB8);
static_assert(offsetof(AmbienceEmitters, numTriggers) == 0x13D8);

// the stream emitter's candidates (0x20 bytes)
struct StreamCandidate {
    CVector pos;          // +0
    float volume;         // +0xC
    uint8_t flags;        // +0x10 1 valid, 2 a mission's, 4, 8 the other play function
    uint8_t pad11[3];
    int32_t typeBits;     // +0x14 (the low four bits)
    int32_t soundId;      // +0x18
    uint16_t u1C;         // +0x1C
    uint16_t u1E;         // +0x1E
};
static_assert(sizeof(StreamCandidate) == 0x20);

struct StreamEmitter {
    float bestDistSq;            // +0
    StreamCandidate candidate;   // +4
    StreamCandidate current;     // +0x24
    StreamCandidate queued[3];   // +0x44
};
static_assert(offsetof(StreamEmitter, queued) == 0x44);

inline void* const TheSounds = reinterpret_cast<void*>(0x00CF3DA0);
inline void* const soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const ambientStream = reinterpret_cast<void*>(0x00CF4324);
inline void* const routes = reinterpret_cast<void*>(0x00BC5E7C);
inline void* const missionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline StreamEmitter* const streamEmitter = reinterpret_cast<StreamEmitter*>(0x00AE5F60);
inline const char* const kTrigEmitFile = reinterpret_cast<const char*>(0x0091C858);   // "audio\config\trigemit.txt"
inline const char* const kReadMode = reinterpret_cast<const char*>(0x008FF548);        // "r"
inline const char* const kTrigEmitFormat = reinterpret_cast<const char*>(0x0091C848);  // "%s\t%s\t%s\t%s\n"
inline const char* const kBadMissionEmitter = reinterpret_cast<const char*>(0x0091C880);
GAME_VAR(AmbienceEmitters*, theEmitters, 0x00CF4278);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint8_t, soundsPaused, 0x00CF3DA2);
GAME_VAR(void*, playerPed, 0x00C1AEA8);
GAME_VAR(CPool<uint8_t>*, triggerPool, 0x020C7C48);
GAME_VAR(void*, streamSounds, 0x00CF70D4);
GAME_VAR(float, soundTimeStep, 0x00CF60F0);
GAME_ARRAY(uint8_t, emitterTypes, 0x00AE6CDE);  // 0xB8-byte entries: their range (a word) first

GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, re::Ptr32<void>* sound, void* value);
GAME_FN(uint8_t, __thiscall, Sound_IsActive, 0x0059CF10, void* sound);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, re::Ptr32<void>* ref, void* entity);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int a3, const void* pos2,
        int a5, int a6, int a7, int a8, void* callback, void* entity, int a11, int a12, float a13, int a14);
GAME_FN(void, __thiscall, SoundBanks_Unload, 0x0059A420, void* banks, int bank, int, int);
GAME_FN(void, __thiscall, SoundBanks_Load, 0x0059A3E0, void* banks, int bank, int, int);
GAME_FN(uint8_t, __thiscall, SoundBanks_IsLoaded, 0x005995F0, void* banks, int bank);
GAME_FN(uint8_t, __thiscall, SoundBanks_Fn598FB0, 0x00598FB0, void* banks, uint32_t hash);
GAME_FN(int, __thiscall, Sounds_GetId, 0x00595B80, void* sounds, const char* name);
GAME_FN(void, __thiscall, Sounds_Fn5960A0, 0x005960A0, void* sounds, uint32_t hash);
GAME_FN(int, __thiscall, Sounds_FindTrigger, 0x00596120, void* sounds, uint32_t hash);
GAME_FN(int, __thiscall, Sounds_GetSoundId, 0x005951A0, void* sounds, int index);
GAME_FN(uint8_t, __cdecl, Condition_IsMet, 0x00593DF0, int condition);
GAME_FN(void, __cdecl, Condition_Parse, 0x00594070, const char* text, uint8_t* out);
GAME_FN(void*, __cdecl, CH_FileOpen, 0x0042D260, const char* name, const char* mode, int flag);
GAME_FN(uint8_t, __cdecl, CH_FileReadLine, 0x0042D330, void* file, char* buffer, int size);
GAME_FN(void, __cdecl, CH_FileClose, 0x0042D450, void* file);
template <typename... A>
inline int __cdecl crt_sscanf(const char* s, const char* format, A... rest) { return crt::Sscanf(s, format, rest...); }
inline int __cdecl crt_stricmp(const char* a, const char* b) { return crt::Stricmp(a, b); }
GAME_FN(uint32_t, __cdecl, HashName, 0x00576ED0, const char* name);
GAME_FN(const char*, __cdecl, GetAreaName, 0x004305B0, int area);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(void, __thiscall, RoutePoint_Construct, 0x006D57E0, RoutePoint* point);
GAME_FN(int, __thiscall, Routes_Find, 0x006D63C0, void* routes, const char* name);
GAME_FN(void*, __thiscall, Routes_Get, 0x006D6450, void* routes, int index);
GAME_FN(RoutePoint*, __thiscall, Route_First, 0x006D59B0, void* route);
GAME_FN(RoutePoint*, __thiscall, Route_Next, 0x006D59F0, void* route);
GAME_FN(CVector*, __cdecl, GetCameraPos, 0x00595630, CVector* out);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(CVector*, __thiscall, Placeable_GetPosition, 0x00415460, void* placeable);
GAME_FN(uint8_t, __thiscall, Area_Contains, 0x006D6FA0, void* area, const CVector* point);
GAME_FN(int32_t*, __thiscall, MissionMgr_Current, 0x006AA790, void* mgr);
GAME_FN(uint8_t, __thiscall, MissionSlot_Is, 0x006AA410, void* slot, int id);
GAME_FN(uint8_t, __thiscall, MissionMgr_Fn6AA7E0, 0x006AA7E0, void* mgr);
GAME_FN(uint8_t, __cdecl, IsHourBetween, 0x00425E40, int start, int end, int, int);
GAME_FN(uint8_t, __cdecl, IsPlayerInFastVehicle, 0x004469A0);
GAME_FN(void, __cdecl, DebugPrint, 0x0049A650, int, const char*);  // (the empty function)
GAME_FN(int, __thiscall, Stream_GetState, 0x0058EDC0, void* stream);
GAME_FN(void*, __thiscall, Stream_Fn58EE00, 0x0058EE00, void* stream);
GAME_FN(void, __thiscall, StreamThing_Fn5A6CE0, 0x005A6CE0, void* thing, StreamCandidate* candidate, int type);
GAME_FN(int, __thiscall, StreamSounds_Fn5A6DD0, 0x005A6DD0, void* sounds, int soundId);
GAME_FN(void, __thiscall, Stream_Play590470, 0x00590470, void* stream, int soundId, int, float volume, int flag, int,
        int, int);
GAME_FN(void, __thiscall, Stream_Play590570, 0x00590570, void* stream, int soundId, int, float volume, int flag, int,
        int, int);
GAME_FN(void, __thiscall, Stream_Stop, 0x005913D0, void* stream, int channel, int fadeMs);
GAME_FN(int, __cdecl, SFX_Fn59E3E0, 0x0059E3E0, float distance, int);
GAME_FN(float, __cdecl, Audio_Fn59E4F0, 0x0059E4F0, float value, int);
GAME_FN(float, __thiscall, Sound_Fn59D3F0, 0x0059D3F0, void* sound);
GAME_FN(void, __thiscall, Sound_GetVoices, 0x0059CF20, void* sound, void* list);
GAME_FN(uint8_t, __cdecl, Voice_Fn5AF530, 0x005AF530, int voice);
GAME_FN(void, __cdecl, Voice_Fn5AF870, 0x005AF870, int voice, int volume, float);
GAME_FN(void, __cdecl, Voice_Fn5AF7B0, 0x005AF7B0, int voice, int volume, int, int);
GAME_FN(float, __cdecl, Audio_Fn5A7F00, 0x005A7F00, float, float);
GAME_FN(void, __cdecl, Voice_Fn5AF660, 0x005AF660, int voice, float);

// (the manager's own, called where modes apply)
GAME_FN(uint8_t, __thiscall, Emitters_HasSlot, 0x00586920, AmbienceEmitters* self, uint32_t hash, int soundId);
GAME_FN(int, __thiscall, Emitters_FindFreeSlot, 0x005868D0, AmbienceEmitters* self);
GAME_FN(void, __thiscall, Slot_Play, 0x00586AF0, EmitterSlot* slot, uint32_t hash, int index, int soundId, int condition);
GAME_FN(AmbientEmitter*, __thiscall, Emitter_Construct, 0x00586B50, AmbientEmitter* self);
GAME_FN(void, __thiscall, Emitter_Set, 0x00586BC0, AmbientEmitter* self, int id, CVector pos, int index, int soundId,
        int bank, int rangeType, int hourStart, int hourEnd, int condition, int flag1B, int missionId);
GAME_FN(void, __thiscall, Emitter_Reset, 0x00586C50, AmbientEmitter* self);
GAME_FN(int, __thiscall, Emitters_FindFreeEmitter, 0x00586CE0, AmbienceEmitters* self);
GAME_FN(void, __thiscall, Emitters_StopSlot, 0x00586D60, AmbienceEmitters* self, uint32_t hash, int soundId);
GAME_FN(void, __thiscall, Emitters_Remove, 0x00586E00, AmbienceEmitters* self, int id);
GAME_FN(uint8_t, __thiscall, Emitters_FindAt, 0x00586E40, AmbienceEmitters* self, float x, float y, float z);
GAME_FN(void, __thiscall, Emitters_LoadTriggers, 0x00586970, AmbienceEmitters* self);
GAME_FN(void, __thiscall, Emitters_StopAll, 0x00587250, AmbienceEmitters* self);
GAME_FN(void, __thiscall, Emitters_PlaySlot, 0x00587300, AmbienceEmitters* self, uint32_t hash, int soundId, int condition);
GAME_FN(void, __thiscall, Emitters_Add, 0x00587340, AmbienceEmitters* self, int id, float x, float y, float z, int soundId,
        int bank, int rangeType, int hourStart, int hourEnd, int condition, int flag1B, int missionId);
GAME_FN(float, __stdcall, DistanceFromCamera, 0x00587490, CVector pos);
GAME_FN(void, __thiscall, Emitter_Update, 0x00587E50, AmbientEmitter* self);
GAME_FN(void, __thiscall, Slot_Update, 0x00587C20, EmitterSlot* self);
GAME_FN(void, __thiscall, Emitters_UpdateTriggers, 0x00587B20, AmbienceEmitters* self);
GAME_FN(void, __thiscall, Emitters_UpdateMission, 0x00587A10, AmbienceEmitters* self);
GAME_FN(void, __thiscall, StreamEmitter_Offer, 0x005870D0, StreamEmitter* self, int soundId, float distSq,
        const CVector* pos, float volume, int type, int mission, int u1C, int u1E, int flag);

constexpr uintptr_t kSoundCallback = 0x00587CE0;
constexpr float kFree = 666.666f;  // (0x4426AAA0)

float Range(int type) {
    return float(*reinterpret_cast<const uint16_t*>(emitterTypes + type * 0xB8));
}

template <typename R>
R vcall(void* object, int offset) {
    void* fn = (*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object))[offset / 4];
    return reinterpret_cast<R(__thiscall*)(void*)>(fn)(object);
}

// a sound told to stop (when it's playing or says so)
void StopSound(void* sound) {
    if (sound && (Sound_IsActive(sound) || vcall<uint8_t>(sound, 0x50))) {
        static_cast<uint8_t*>(sound)[0x5C] = 1;
    }
}

// "SP_" and the current area's name: the area's route of emitters
int FindAreaRoute(AmbienceEmitters* self) {
    char name[64];
    std::memcpy(name, "SP_", 4);
    std::strcat(name, GetAreaName(currArea));
    int index = Routes_Find(routes, name);
    self->routeIndex = index;
    return index;
}

// a route point's time, condition and mission checks (flag 8 of a point of the running
// mission: `quieter` set)
bool PointActive(const RoutePoint* point, bool* quieter) {
    bool timeOk;
    if (point->hourStart == point->hourEnd) {
        timeOk = true;
    } else {
        timeOk = IsHourBetween(point->hourStart, point->hourEnd, 0, 0) != 0;
    }
    uint8_t conditionOk = Condition_IsMet(point->condition);
    bool missionOk;
    int32_t mission = point->missionId;
    if (mission == -2) {
        missionOk = MissionMgr_Fn6AA7E0(missionMgr) == 0;
    } else if (mission == -1) {
        missionOk = true;
    } else if (mission == -1000000) {
        missionOk = false;
        DebugPrint(0, kBadMissionEmitter);
    } else {
        int32_t* current = MissionMgr_Current(missionMgr);
        if (point->missionId == *current &&
            MissionSlot_Is(MissionMgr_Current(missionMgr), point->missionId)) {
            missionOk = (point->flags & 4) == 0;
            if (quieter && (point->flags & 8)) {
                *quieter = true;
            }
        } else {
            missionOk = (point->flags & 0xC) != 0;
        }
    }
    return timeOk && conditionOk && missionOk;
}

float LengthSq(const CVector& d) {
    return d.x * d.x + d.y * d.y + d.z * d.z;
}

// ---- the manager ---------------------------------------------------------------------

// 0x586820 (thiscall): a mission's emitter in the first free of the 16 (none past them)
void __fastcall Emitters_AddMissionEmitter(AmbienceEmitters* self, re::Edx, float x, float y, float z, int soundId, int type,
                                           int condition) {
    int i = 0;
    while (i < 16 && self->missionEmitters[i].soundId != 0) {
        i++;
    }
    if (i == 16) {
        return;
    }
    RoutePoint& point = self->missionEmitters[i];
    point.pos.x = x;
    point.condition = uint8_t(condition);
    point.pos.y = y;
    point.type = int8_t(type);
    point.pos.z = z;
    point.hourStart = 0;
    point.hourEnd = 23;
    point.missionId = -1;
    point.soundId = soundId;
}

// 0x5868D0 (thiscall): a free slot (the hint first), -1 when none
int __fastcall Emitters_FindFreeSlotImpl(AmbienceEmitters* self, re::Edx) {
    int hint = self->freeSlotHint;
    if (hint > -1) {
        self->freeSlotHint = -1;
        return hint;
    }
    if (self->numSlots <= 0) {
        return -1;
    }
    for (int i = 0; i < self->numSlots; i++) {
        if (self->slots[i]->hash == 0) {
            return i;
        }
    }
    return -1;
}

// 0x586920 (thiscall): whether a slot plays this trigger's sound
uint8_t __fastcall Emitters_HasSlotImpl(AmbienceEmitters* self, re::Edx, uint32_t hash, int soundId) {
    for (int i = 0; i < self->numSlots; i++) {
        EmitterSlot* slot = self->slots[i];
        if (slot->hash == hash && soundId == slot->soundId) {
            return 1;
        }
    }
    return 0;
}

// 0x586970 (thiscall): reads audio\config\trigemit.txt: "trigger area sound condition"
// lines ('#' comments, shorter than 8 skipped); unknown areas skipped
void __fastcall Emitters_LoadTriggersImpl(AmbienceEmitters* self, re::Edx) {
    int count = 0;
    void* file = CH_FileOpen(kTrigEmitFile, kReadMode, 1);
    char line[512];
    char sound[64], area[64], condition[64], name[64];
    TriggerEmitter* entry = self->triggers;
    while (CH_FileReadLine(file, line, 0x200)) {
        if (line[0] == '#' || std::strlen(line) < 8) {
            continue;
        }
        crt_sscanf(line, kTrigEmitFormat, name, area, sound, condition);
        int soundId = Sounds_GetId(TheSounds, sound);
        uint32_t hash = HashName(name);
        int areaIndex = 0;
        for (; areaIndex < 0x40; areaIndex++) {
            if (crt_stricmp(area, GetAreaName(areaIndex)) == 0) {
                break;
            }
        }
        if (areaIndex >= 0x40 || areaIndex == -1) {
            continue;
        }
        entry->soundId = soundId;
        entry->hash = hash;
        Sounds_Fn5960A0(TheSounds, hash);
        entry->area = areaIndex;
        Condition_Parse(condition, &entry->condition);
        count++;
        entry++;
    }
    self->numTriggers = count;
    CH_FileClose(file);
}

// 0x586AF0 (thiscall): a slot's loop started (a negative index does nothing)
void __fastcall Slot_PlayImpl(EmitterSlot* self, re::Edx, uint32_t hash, int index, int soundId, int condition) {
    if (index <= -1) {
        return;
    }
    self->index = index;
    self->condition = uint8_t(condition);
    self->hash = hash;
    void* sound = Sounds_PlayAt(TheSounds, soundId, nullptr, 1, nullptr, 0, 0x78, 0, 0, nullptr, nullptr, 6, 1, 1.0f, 0);
    Sound_Set(&self->sound, sound);
    self->soundId = soundId;
}

// 0x586B50 (thiscall): an emitter constructed, free
AmbientEmitter* __fastcall Emitter_ConstructImpl(AmbientEmitter* self, re::Edx) {
    self->pos.x = 0.0f;
    self->pos.y = 0.0f;
    self->pos.z = 0.0f;
    self->sound = nullptr;
    self->entity = nullptr;
    Sound_Set(&self->sound, nullptr);
    SetRef(&self->entity, nullptr);
    self->pos.z = kFree;
    self->pos.y = kFree;
    self->bank = -1;
    self->pos.x = kFree;
    self->index = -1;
    self->range = 1000.0f;
    self->condition = 0;
    self->state = 0;
    return self;
}

// 0x586BC0 (thiscall): an emitter set up (waiting for its bank when it has one)
void __fastcall Emitter_SetImpl(AmbientEmitter* self, re::Edx, int id, CVector pos, int index, int soundId, int bank,
                                int rangeType, int hourStart, int hourEnd, int condition, int flag1B, int missionId) {
    if (index <= -1) {
        return;
    }
    self->missionId = missionId;
    self->hourEnd = uint8_t(hourEnd);
    self->hourStart = uint8_t(hourStart);
    self->index = int16_t(index);
    SetRef(&self->entity, playerPed);
    self->pos = pos;
    self->rangeType = rangeType;
    self->condition = uint8_t(condition);
    self->flag1B = uint8_t(flag1B);
    self->bank = int16_t(bank);
    self->soundId = soundId;
    self->id = id;
    self->state = 2;
    if (int16_t(bank) != -1) {
        self->state = 1;
    }
}

// 0x586C50 (thiscall): an emitter stopped and freed (its bank unloaded unless it never
// loaded)
void __fastcall Emitter_ResetImpl(AmbientEmitter* self, re::Edx) {
    SetRef(&self->entity, nullptr);
    self->pos.x = kFree;
    self->pos.y = kFree;
    self->pos.z = kFree;
    self->range = 1000.0f;
    StopSound(self->sound);
    self->index = -1;
    Sound_Set(&self->sound, nullptr);
    self->condition = 0;
    self->missionId = -1;
    if (self->bank != -1 && self->state != 1) {
        SoundBanks_Unload(soundBanks, self->bank, 1, 1);
    }
    self->state = 0;
}

// 0x586CE0 (thiscall): a free emitter: the hint, else one at 666.666 with no sound
// (compared with the double 666.666, which the float never equals: only the hint works)
int __fastcall Emitters_FindFreeEmitterImpl(AmbienceEmitters* self, re::Edx) {
    int hint = self->freeEmitterHint;
    if (hint > -1) {
        self->freeEmitterHint = -1;
        return hint;
    }
    for (int i = 0; i < self->numEmitters; i++) {
        AmbientEmitter* e = self->emitters[i];
        if (e && double(e->pos.x) == 666.666 && double(e->pos.y) == 666.666 && double(e->pos.z) == 666.666 &&
            !e->sound) {
            return i;
        }
    }
    return -1;
}

// 0x586D60 (thiscall): a trigger's slot stopped (the freed index goes to the emitters'
// hint, not the slots')
void __fastcall Emitters_StopSlotImpl(AmbienceEmitters* self, re::Edx, uint32_t hash, int soundId) {
    for (int i = 0; i < self->numSlots; i++) {
        EmitterSlot* slot = self->slots[i];
        if (slot->hash == hash && slot->soundId == soundId) {
            if (i <= -1) {
                return;
            }
            slot->hash = 0;
            slot->soundId = 0;
            slot->condition = 0;
            StopSound(slot->sound);
            slot->index = -1;
            Sound_Set(&slot->sound, nullptr);
            slot->range = 1000.0f;
            self->freeEmitterHint = i;
            return;
        }
    }
}

// 0x586E00 (thiscall): the emitter with this id stopped
void __fastcall Emitters_RemoveImpl(AmbienceEmitters* self, re::Edx, int id) {
    for (int i = 0; i < self->numEmitters; i++) {
        if (id == self->emitters[i]->id) {
            if (i > -1) {
                Emitter_Reset(self->emitters[i]);
                self->freeEmitterHint = i;
            }
            return;
        }
    }
}

// 0x586E40 (thiscall): whether an emitter is at exactly this position
uint8_t __fastcall Emitters_FindAtImpl(AmbienceEmitters* self, re::Edx, float x, float y, float z) {
    for (int i = 0; i < self->numEmitters; i++) {
        const CVector& pos = self->emitters[i]->pos;
        if (pos.x == x && pos.y == y && pos.z == z) {
            return 1;
        }
    }
    return 0;
}

// 0x586ED0 (thiscall): the constructor (from TheSounds' set-up: 25 emitters of 0x38
// bytes, 10 slots of 0x18)
AmbienceEmitters* __fastcall Emitters_Construct(AmbienceEmitters* self, re::Edx, int numEmitters, int emitterSize, int numSlots,
                                                int slotSize) {
    for (int i = 0; i < 16; i++) {
        RoutePoint_Construct(&self->missionEmitters[i]);
    }
    self->route = nullptr;
    self->unknownA4 = 0;
    self->numEmitters = numEmitters;
    self->numSlots = numSlots;
    self->emitterSize = emitterSize;
    self->slotSize = slotSize;
    self->disabled = 0;
    for (int i = 0; i < numEmitters; i++) {
        void* p = Malloc(0x38);
        self->emitters[i] = p ? Emitter_Construct(static_cast<AmbientEmitter*>(p)) : nullptr;
    }
    for (int i = 0; i < numSlots; i++) {
        EmitterSlot* slot = static_cast<EmitterSlot*>(Malloc(0x18));
        if (slot) {
            slot->sound = nullptr;
            Sound_Set(&slot->sound, nullptr);
            slot->hash = 0;
            slot->index = -1;
            slot->condition = 0;
        }
        self->slots[i] = slot;
    }
    self->freeEmitterHint = 0;
    self->freeSlotHint = 0;
    self->frame = 0;
    for (int i = 0; i < 16; i++) {
        self->missionEmitters[i].soundId = 0;
    }
    return self;
}

// 0x586FE0 (thiscall): the trigger emitters read again
void __fastcall Emitters_Reload(AmbienceEmitters* self, re::Edx) {
    self->numTriggers = 0;
    self->unknownA4 = 0;
    Emitters_LoadTriggers(self);
}

// ---- the stream emitter --------------------------------------------------------------

// 0x587000 (thiscall): a candidate queued in the first free of three
uint8_t __fastcall StreamEmitter_Queue(StreamEmitter* self, re::Edx, const StreamCandidate* candidate) {
    for (int i = 0; i < 3; i++) {
        if (!(self->queued[i].flags & 1)) {
            self->queued[i] = *candidate;
            self->queued[i].flags |= 1;
            return 1;
        }
    }
    return 0;
}

// 0x587040 (thiscall): a queued candidate dropped
uint8_t __fastcall StreamEmitter_Unqueue(StreamEmitter* self, re::Edx, int soundId) {
    for (int i = 0; i < 3; i++) {
        if ((self->queued[i].flags & 1) && self->queued[i].soundId == soundId) {
            self->queued[i].flags &= 0xFE;
            return 1;
        }
    }
    return 0;
}

// 0x587080 (thiscall): a candidate taken when it's closer (a mission's only over a
// mission's)
void __fastcall StreamEmitter_OfferCandidate(StreamEmitter* self, re::Edx, const StreamCandidate* candidate, float distSq) {
    if (!(distSq < self->bestDistSq)) {
        return;
    }
    uint8_t flags = self->candidate.flags;
    if ((flags & 1) && (flags & 2) && !(candidate->flags & 2)) {
        return;
    }
    self->candidate = *candidate;
    self->bestDistSq = distSq;
    self->candidate.flags |= 1;
}

// 0x5870D0 (thiscall): a sound offered as the candidate when it's closer
void __fastcall StreamEmitter_OfferImpl(StreamEmitter* self, re::Edx, int soundId, float distSq, const CVector* pos, float volume,
                                        int type, int mission, int u1C, int u1E, int flag) {
    if (!(distSq < self->bestDistSq)) {
        return;
    }
    uint8_t flags = self->candidate.flags;
    if ((flags & 1) && (flags & 2) && uint8_t(mission) == 0) {
        return;
    }
    flags = uint8_t((((uint8_t(mission) * 2) ^ flags) & 2) ^ flags);
    self->candidate.flags = flags;
    self->candidate.soundId = soundId;
    self->candidate.pos = *pos;
    self->candidate.volume = volume;
    self->bestDistSq = distSq;
    self->candidate.typeBits ^= (self->candidate.typeBits ^ type) & 0xF;
    self->candidate.u1C = uint16_t(u1C);
    self->candidate.u1E = uint16_t(u1E);
    self->candidate.flags = uint8_t(((flag & 1) << 2) | (flags & 0xF3) | 1);
}

// 0x587170 (thiscall): an owned block freed
void __fastcall OwnedBlock_Free(re::Ptr32<void>* self, re::Edx) {
    if (*self) {
        Free(*self);
        *self = nullptr;
    }
}

// 0x587190 (thiscall): the mission's emitter at a position (within 0.0001) with this sound
// removed
void __fastcall Emitters_RemoveMissionEmitter(AmbienceEmitters* self, re::Edx, float x, float y, float z, int soundId) {
    for (int i = 0; i < 16; i++) {
        RoutePoint& point = self->missionEmitters[i];
        if (point.soundId == 0) {
            continue;
        }
        float dx = std::fabs(point.pos.x - x);
        if (!(double(dx) < 0.0001)) {
            continue;
        }
        float dy = std::fabs(point.pos.y - y);
        if (!(double(dy) < 0.0001)) {
            continue;
        }
        float dz = std::fabs(point.pos.z - z);
        if (!(double(dz) < 0.0001)) {
            continue;
        }
        if (soundId == point.soundId) {
            Emitters_Remove(self, -1 - i);
            point.soundId = 0;
        }
    }
}

// 0x587250 (thiscall): every slot and emitter stopped
void __fastcall Emitters_StopAllImpl(AmbienceEmitters* self, re::Edx) {
    for (int i = 0; i < self->numSlots; i++) {
        EmitterSlot* slot = self->slots[i];
        if (slot && slot->index != -1) {
            slot->hash = 0;
            slot->soundId = 0;
            slot->condition = 0;
            StopSound(slot->sound);
            slot->index = -1;
            Sound_Set(&slot->sound, nullptr);
            slot->range = 1000.0f;
        }
    }
    for (int i = 0; i < self->numEmitters; i++) {
        AmbientEmitter* e = self->emitters[i];
        if (e && e->index != -1) {
            Emitter_Reset(e);
        }
    }
}

// 0x587300 (thiscall): a trigger's loop started in a free slot unless it plays already
void __fastcall Emitters_PlaySlotImpl(AmbienceEmitters* self, re::Edx, uint32_t hash, int soundId, int condition) {
    if (Emitters_HasSlot(self, hash, soundId)) {
        return;
    }
    int index = Emitters_FindFreeSlot(self);
    if (index > -1) {
        Slot_Play(self->slots[index], hash, index, soundId, condition);
    }
}

// 0x587340 (thiscall): an emitter added unless one is at that position already
void __fastcall Emitters_AddImpl(AmbienceEmitters* self, re::Edx, int id, float x, float y, float z, int soundId, int bank,
                                 int rangeType, int hourStart, int hourEnd, int condition, int flag1B, int missionId) {
    if (Emitters_FindAt(self, x, y, z)) {
        return;
    }
    int index = Emitters_FindFreeEmitter(self);
    if (index > -1) {
        Emitter_Set(self->emitters[index], id, CVector{x, y, z}, index, soundId, bank, rangeType, hourStart, hourEnd,
                    condition, flag1B, missionId);
    }
}

// 0x5873C0 (thiscall): an area route point's emitter enabled or disabled (and its
// emitter stopped when disabling)
void __fastcall Emitters_Enable(AmbienceEmitters* self, re::Edx, uint32_t hash, int on) {
    if (!re::Flag(on)) {
        Emitters_Remove(self, int(hash));
    }
    if (FindAreaRoute(self) == -1) {
        return;
    }
    self->route = Routes_Get(routes, self->routeIndex);
    for (RoutePoint* point = Route_First(self->route); point; point = Route_Next(self->route)) {
        if (point->hash == hash) {
            point->flags ^= ((re::Flag(on) ? 0 : 1) << 4 ^ point->flags) & 0x10;
            return;
        }
    }
}

// 0x587490 (a thiscall that doesn't read ecx: stdcall): the distance of a point from the
// camera
float __stdcall DistanceFromCameraImpl(CVector pos) {
    CVector cam = {0.0f, 0.0f, 0.0f};
    CVector tmp;
    cam = *GetCameraPos(&tmp);
    CVector d;
    VectorSub(&d, &pos, &cam);
    float sum = LengthSq(d);
    return float(std::sqrt(double(std::fabs(sum))));
}

// 0x587520 (thiscall): everything stopped and deleted (the emitters' loop runs twice)
void __fastcall Emitters_Shutdown(AmbienceEmitters* self, re::Edx) {
    Emitters_StopAll(self);
    for (int i = 0; i < self->numSlots; i++) {
        EmitterSlot* slot = self->slots[i];
        if (slot) {
            Sound_Set(&slot->sound, nullptr);
            Free(slot);
        }
        self->slots[i] = nullptr;
    }
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < self->numEmitters; i++) {
            AmbientEmitter* e = self->emitters[i];
            if (e) {
                SetRef(&e->entity, nullptr);
                Sound_Set(&e->sound, nullptr);
                Free(e);
            }
            self->emitters[i] = nullptr;
        }
    }
}

// 0x5875F0 (thiscall): the nearest active area route point within sqrt 2 of an entity
// enabled or disabled
void __fastcall Emitters_EnableNear(AmbienceEmitters* self, re::Edx, void* entity, int on) {
    if (FindAreaRoute(self) == -1) {
        return;
    }
    float best = 100000.0f;
    RoutePoint* nearest = nullptr;
    self->route = Routes_Get(routes, self->routeIndex);
    RoutePoint* point = Route_First(self->route);
    while (point) {
        RoutePoint* next = Route_Next(self->route);
        CVector d;
        VectorSub(&d, &point->pos, Placeable_GetPosition(entity));
        float distSq = LengthSq(d);
        if (distSq < 2.0f && distSq < best && PointActive(point, nullptr)) {
            best = distSq;
            nearest = point;
        }
        point = next;
    }
    if (nearest) {
        nearest->flags ^= (uint8_t(on) << 4 ^ nearest->flags) & 0x10;
    }
}

// 0x587800 (thiscall): the stream emitter's update: the queued candidates offered (by
// the player's distance), then the closest one streamed (or the stream stopped)
void __fastcall StreamEmitter_Update(StreamEmitter* self, re::Edx) {
    for (int i = 0; i < 3; i++) {
        StreamCandidate* q = &self->queued[i];
        if (!(q->flags & 1)) {
            continue;
        }
        uint8_t* ped = static_cast<uint8_t*>(playerPed);
        uint8_t* matrix = *reinterpret_cast<re::Ptr32<uint8_t>*>(ped + 0x14);
        const CVector* playerPos = reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : ped + 4);
        CVector d;
        CVector* r = VectorSub(&d, playerPos, &q->pos);
        float distSq = r->x * r->x + r->y * r->y + r->z * r->z;
        if (distSq < self->bestDistSq) {
            uint8_t flags = self->candidate.flags;
            if (!(flags & 1) || !(flags & 2) || (q->flags & 2)) {
                self->candidate = *q;
                self->bestDistSq = distSq;
                self->candidate.flags |= 1;
            }
        }
    }
    if (Stream_GetState(ambientStream) <= 2 && (self->candidate.flags & 1)) {
        if ((self->current.flags & 1) && self->current.soundId == self->candidate.soundId &&
            Stream_GetState(ambientStream) == 2) {
            int type = (self->candidate.typeBits << 28) >> 28;
            StreamThing_Fn5A6CE0(Stream_Fn58EE00(ambientStream), &self->candidate, type);
            self->bestDistSq = 1e6f;
            self->current = self->candidate;
            self->candidate.flags &= 0xFE;
            return;
        }
        if (!IsPlayerInFastVehicle()) {
            int x = StreamSounds_Fn5A6DD0(streamSounds, self->candidate.soundId);
            uint8_t flags = self->candidate.flags;
            if (flags & 8) {
                Stream_Play590570(ambientStream, self->candidate.soundId, x, self->candidate.volume, (flags >> 2) & 1, 2,
                                  self->candidate.u1C, self->candidate.u1E);
            } else {
                Stream_Play590470(ambientStream, self->candidate.soundId, x, self->candidate.volume, (flags >> 2) & 1, 2,
                                  self->candidate.u1C, self->candidate.u1E);
            }
            self->bestDistSq = 1e6f;
            self->current = self->candidate;
            self->candidate.flags &= 0xFE;
            return;
        }
    }
    if (self->current.flags & 1) {
        int state = Stream_GetState(ambientStream);
        if (state == 2) {
            Stream_Stop(ambientStream, state, -1);
        }
        self->current.flags &= 0xFE;
    }
    self->candidate.flags &= 0xFE;
    self->bestDistSq = 1e6f;
}

// 0x587A10 (thiscall): the mission's emitters within their type's range of the camera added
void __fastcall Emitters_UpdateMissionImpl(AmbienceEmitters* self, re::Edx) {
    CVector cam;
    GetCameraPos(&cam);
    for (int i = 0; i < 16; i++) {
        RoutePoint& point = self->missionEmitters[i];
        if (point.soundId == 0) {
            continue;
        }
        CVector pos = point.pos;
        CVector d;
        VectorSub(&d, &pos, &cam);
        int type = point.type;
        float dist = float(std::sqrt(double(std::fabs(LengthSq(d)))));
        if (dist < Range(type)) {
            Emitters_Add(self, -1 - i, pos.x, pos.y, pos.z, point.soundId, -1, type, uint8_t(point.hourStart),
                         uint8_t(point.hourEnd), point.condition, 0, point.missionId);
        }
    }
}

// 0x587B20 (thiscall): the trigger emitters of this area whose trigger holds the camera
// started
void __fastcall Emitters_UpdateTriggersImpl(AmbienceEmitters* self, re::Edx) {
    CVector cam = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < self->numTriggers; i++) {
        TriggerEmitter& entry = self->triggers[i];
        if (entry.area != currArea || !Condition_IsMet(entry.condition)) {
            continue;
        }
        int index = Sounds_FindTrigger(TheSounds, entry.hash);
        if (index == -1 || index < 0) {
            continue;
        }
        CPool<uint8_t>* pool = triggerPool;
        if (pool->flags[index] & 0x80) {
            continue;
        }
        uint8_t* trigger = pool->objects + pool->elemSize * index;
        if (!trigger) {
            continue;
        }
        CVector tmp;
        cam = *GetCameraPos(&tmp);
        if (Area_Contains(trigger, &cam) && SoundBanks_Fn598FB0(soundBanks, entry.hash)) {
            Emitters_PlaySlot(self, entry.hash, entry.soundId, entry.condition);
        }
    }
}

// 0x587C20 (thiscall): a slot stopped when the camera left its trigger, the manager is
// disabled or its condition fails (a trigger slot that's gone passes a null area)
void __fastcall Slot_UpdateImpl(EmitterSlot* self, re::Edx) {
    CVector cam = {0.0f, 0.0f, 0.0f};
    CVector tmp;
    cam = *GetCameraPos(&tmp);
    int index = Sounds_FindTrigger(TheSounds, self->hash);
    if (index == -1) {
        return;
    }
    uint8_t* trigger;
    if (index < 0) {
        trigger = nullptr;
    } else {
        CPool<uint8_t>* pool = triggerPool;
        trigger = (pool->flags[index] & 0x80) ? nullptr : pool->objects + pool->elemSize * index;
    }
    if (!Area_Contains(trigger, &cam) || theEmitters->disabled) {
        Emitters_StopSlot(theEmitters, self->hash, self->soundId);
    }
    if (!Condition_IsMet(self->condition)) {
        Emitters_StopSlot(theEmitters, self->hash, self->soundId);
    }
}

struct VoiceList {   // (the game's layout)
    re::Ptr32<int> data;
    uint32_t unknown4;
    uint32_t count;
};

// 0x587CE0: the emitters' sound callback (cdecl): the volume by distance, stopped at none
void __cdecl SoundCallback(float distance, float timeStep, int, void* sound) {
    int raw = SFX_Fn59E3E0(distance, vcall<int>(sound, 0x68));
    float volume = float(uint32_t(raw));
    if (volume <= 0.0f) {
        static_cast<uint8_t*>(sound)[0x5C] = 1;
    }
    float scale = Audio_Fn59E4F0(volume, 3);
    float level = Sound_Fn59D3F0(sound) * scale;
    VoiceList voices = {nullptr, 0, 0};
    Sound_GetVoices(sound, &voices);
    for (uint32_t i = 0; i < voices.count; i++) {
        if (voices.data[i] == -1) {
            continue;
        }
        int b8 = vcall<int>(sound, 0xB8);
        if (Voice_Fn5AF530(voices.data[i])) {
            Voice_Fn5AF870(voices.data[i], int(level), 0.0f);
        } else {
            Voice_Fn5AF7B0(voices.data[i], int(level), b8, 1);
        }
        float value = Audio_Fn5A7F00(timeStep, soundTimeStep);
        Voice_Fn5AF660(voices.data[i], value);
    }
    if (voices.data) {
        Free(voices.data);
    }
}

// 0x587E50 (thiscall): an emitter's update: its bank loaded, then its sound started, then
// stopped when out of range, out of hours, off-condition or its mission is over
void __fastcall Emitter_UpdateImpl(AmbientEmitter* self, re::Edx) {
    switch (self->state) {
    case 1:
        if (!IsPlayerInFastVehicle() && !soundsPaused) {
            SoundBanks_Load(soundBanks, self->bank, 1, 1);
            self->state = 2;
        }
        return;
    case 2: {
        if (self->bank != -1 && !SoundBanks_IsLoaded(soundBanks, self->bank)) {
            return;
        }
        void* sound = Sounds_PlayAt(TheSounds, self->soundId, &self->pos, self->rangeType, nullptr, 0, 0x7D, 0x7E, 0x7F,
                                    reinterpret_cast<void*>(kSoundCallback), self->entity, 6, 1, 1.0f, 0);
        Sound_Set(&self->sound, sound);
        if (self->sound) {
            reinterpret_cast<void(__thiscall*)(void*, int)>(static_cast<void*>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(self->sound))[0x98 / 4]))(
                self->sound, self->flag1B);
        }
        self->state = 3;
        return;
    }
    case 3: {
        if (!self->sound) {
            Emitter_Reset(self);
            return;
        }
        AmbienceEmitters* mgr = theEmitters;
        float dist = DistanceFromCamera(self->pos);
        if (Range(self->rangeType) < dist || mgr->disabled) {
            Emitters_Remove(mgr, self->id);
            return;
        }
        if (self->hourStart != self->hourEnd && !IsHourBetween(self->hourStart, self->hourEnd, 0, 0)) {
            Emitters_Remove(theEmitters, self->id);
            return;
        }
        if (!Condition_IsMet(self->condition)) {
            Emitters_Remove(theEmitters, self->id);
        }
        if (self->missionId < 0) {
            return;
        }
        int32_t* current = MissionMgr_Current(missionMgr);
        if (*current == self->missionId && MissionSlot_Is(MissionMgr_Current(missionMgr), self->missionId)) {
            return;
        }
        Emitters_Remove(theEmitters, self->id);
        return;
    }
    }
}

// 0x588030 (thiscall): the update: the emitters and slots, then (unless disabled) the
// triggers', the mission's and the area route's emitters in range added, and the
// closest streamed point offered to the stream emitter
void __fastcall Emitters_Update(AmbienceEmitters* self, re::Edx) {
    self->frame++;
    bool quieter = false;
    if (self->frame > 10) {
        self->frame = 0;
    }
    for (int i = 0; i < self->numEmitters; i++) {
        AmbientEmitter* e = self->emitters[i];
        if (e && double(e->pos.x) != 666.666) {
            Emitter_Update(e);
        }
    }
    for (int i = 0; i < self->numSlots; i++) {
        EmitterSlot* slot = self->slots[i];
        if (!slot || slot->hash == 0) {
            continue;
        }
        if (!slot->sound) {
            slot->hash = 0;
            slot->soundId = 0;
            slot->condition = 0;
            slot->index = -1;
            Sound_Set(&slot->sound, nullptr);
            slot->range = 1000.0f;
        } else {
            Slot_Update(slot);
        }
    }
    if (self->disabled) {
        return;
    }
    Emitters_UpdateTriggers(self);
    Emitters_UpdateMission(self);
    CVector cam;
    GetCameraPos(&cam);
    if (FindAreaRoute(self) == -1) {
        return;
    }
    bool streamFree = Stream_GetState(ambientStream) <= 2;
    float best = 100000.0f;
    uint8_t haveMission = 0;
    RoutePoint* nearest = nullptr;
    self->route = Routes_Get(routes, self->routeIndex);
    RoutePoint* point = Route_First(self->route);
    while (point) {
        RoutePoint* next = Route_Next(self->route);
        CVector pos = point->pos;
        CVector d;
        VectorSub(&d, &pos, &cam);
        float dist = float(std::sqrt(double(std::fabs(LengthSq(d)))));
        if (!(point->flags & 0x10) && dist < Range(point->type) && PointActive(point, &quieter)) {
            if (point->flags & 2) {
                if (streamFree) {
                    if (point->missionId < 0) {
                        if (!haveMission && dist < best) {
                            best = dist;
                            nearest = point;
                        }
                    } else if (!haveMission || dist < best) {
                        nearest = point;
                        best = dist;
                        haveMission = 1;
                    }
                }
            } else {
                int soundId = Sounds_GetSoundId(TheSounds, point->soundIndex);
                Emitters_Add(self, int(point->hash), pos.x, pos.y, pos.z, soundId, uint16_t(point->bank), point->type,
                             uint8_t(point->hourStart), uint8_t(point->hourEnd), point->condition, point->flags & 1,
                             point->missionId);
            }
        }
        point = next;
    }
    if (streamFree && nearest) {
        float volume = float(double(nearest->volume) * 0.009999999776482582);
        if (quieter) {
            volume = float(double(volume) * 0.30000001192092896);
        }
        // (the mission flag goes as a dword whose upper bytes are the stack's)
        StreamEmitter_Offer(streamEmitter, *reinterpret_cast<const int32_t*>(&nearest->soundIndex), best * best,
                            &nearest->pos, volume, nearest->type, haveMission, nearest->u28 << 7, nearest->u29 << 7, 1);
    }
}

} // namespace

#define EMITTERS_REVERSED(name, address, fn)                                                   \
    static void* RE_CONCAT(emitters_original_, __LINE__);                                     \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(emitters_original_, __LINE__), \
             re::Mode::Original)

EMITTERS_REVERSED("AmbienceEmitters::AddMissionEmitter", 0x00586820, &Emitters_AddMissionEmitter);
EMITTERS_REVERSED("AmbienceEmitters::FindFreeSlot", 0x005868D0, &Emitters_FindFreeSlotImpl);
EMITTERS_REVERSED("AmbienceEmitters::HasSlot", 0x00586920, &Emitters_HasSlotImpl);
EMITTERS_REVERSED("AmbienceEmitters::LoadTriggers", 0x00586970, &Emitters_LoadTriggersImpl);
EMITTERS_REVERSED("AmbienceEmitters::Slot::Play", 0x00586AF0, &Slot_PlayImpl);
EMITTERS_REVERSED("AmbienceEmitters::Emitter::Emitter", 0x00586B50, &Emitter_ConstructImpl);
EMITTERS_REVERSED("AmbienceEmitters::Emitter::Set", 0x00586BC0, &Emitter_SetImpl);
EMITTERS_REVERSED("AmbienceEmitters::Emitter::Reset", 0x00586C50, &Emitter_ResetImpl);
EMITTERS_REVERSED("AmbienceEmitters::FindFreeEmitter", 0x00586CE0, &Emitters_FindFreeEmitterImpl); // callers rely on ecx: AmbienceEmitters::Add 0x587340 (turn on together)
EMITTERS_REVERSED("AmbienceEmitters::StopSlot", 0x00586D60, &Emitters_StopSlotImpl);
EMITTERS_REVERSED("AmbienceEmitters::Remove", 0x00586E00, &Emitters_RemoveImpl);
EMITTERS_REVERSED("AmbienceEmitters::FindAt", 0x00586E40, &Emitters_FindAtImpl);
EMITTERS_REVERSED("AmbienceEmitters::AmbienceEmitters", 0x00586ED0, &Emitters_Construct);
EMITTERS_REVERSED("AmbienceEmitters::Reload", 0x00586FE0, &Emitters_Reload);
EMITTERS_REVERSED("StreamEmitter::Queue", 0x00587000, &StreamEmitter_Queue);
EMITTERS_REVERSED("StreamEmitter::Unqueue", 0x00587040, &StreamEmitter_Unqueue);
EMITTERS_REVERSED("StreamEmitter::OfferCandidate", 0x00587080, &StreamEmitter_OfferCandidate);
EMITTERS_REVERSED("StreamEmitter::Offer", 0x005870D0, &StreamEmitter_OfferImpl);
EMITTERS_REVERSED("OwnedBlock::Free_587170", 0x00587170, &OwnedBlock_Free);
EMITTERS_REVERSED("AmbienceEmitters::RemoveMissionEmitter", 0x00587190, &Emitters_RemoveMissionEmitter);
EMITTERS_REVERSED("AmbienceEmitters::StopAll", 0x00587250, &Emitters_StopAllImpl);
EMITTERS_REVERSED("AmbienceEmitters::PlaySlot", 0x00587300, &Emitters_PlaySlotImpl);
EMITTERS_REVERSED("AmbienceEmitters::Add", 0x00587340, &Emitters_AddImpl);
EMITTERS_REVERSED("AmbienceEmitters::Enable", 0x005873C0, &Emitters_Enable);
EMITTERS_REVERSED("AmbienceEmitters::DistanceFromCamera", 0x00587490, &DistanceFromCameraImpl);
EMITTERS_REVERSED("AmbienceEmitters::Shutdown", 0x00587520, &Emitters_Shutdown);
EMITTERS_REVERSED("AmbienceEmitters::EnableNear", 0x005875F0, &Emitters_EnableNear);
EMITTERS_REVERSED("StreamEmitter::Update", 0x00587800, &StreamEmitter_Update);
EMITTERS_REVERSED("AmbienceEmitters::UpdateMission", 0x00587A10, &Emitters_UpdateMissionImpl);
EMITTERS_REVERSED("AmbienceEmitters::UpdateTriggers", 0x00587B20, &Emitters_UpdateTriggersImpl);
EMITTERS_REVERSED("AmbienceEmitters::Slot::Update", 0x00587C20, &Slot_UpdateImpl);
EMITTERS_REVERSED("AmbienceEmitters::SoundCallback", 0x00587CE0, &SoundCallback);
EMITTERS_REVERSED("AmbienceEmitters::Emitter::Update", 0x00587E50, &Emitter_UpdateImpl);
EMITTERS_REVERSED("AmbienceEmitters::Update", 0x00588030, &Emitters_Update);
