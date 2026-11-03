// The interactive music (0x58E060..0x590000): audio\config\IMusic.bin's tracks (0x40
// bytes each at 0xCF3B04, looked up by id through 0xCF3AA8), transitions (0x1E bytes at
// 0xCF3B08) and sound-id lists (0xCF3B0C); each track's condition function (a table at
// 0xAE6288) turning it on with an intensity; the combat tracker that feeds the fight
// music; and the music streams' small getters.

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

// a music track (0x40 bytes)
struct MusicTrack {
    uint8_t id;               // +0
    uint8_t unknown1[5];
    uint8_t unknown6;         // +6
    uint8_t unknown7;
    uint16_t onDelayMin;      // +8
    uint16_t onDelayMax;      // +0xA
    uint16_t offDelayMin;     // +0xC
    uint16_t offDelayMax;     // +0xE
    uint8_t unknown10[2];
    uint8_t layer12;          // +0x12
    uint8_t level;            // +0x13 (0..2: the intensity in thirds)
    uint32_t switchTime;      // +0x14
    float intensity;          // +0x18
    uint8_t unknown1C;
    uint8_t unknown1D;        // +0x1D
    uint8_t active;           // +0x1E
    uint8_t unknown1F;
    struct Layer {            // +0x20 (10 bytes)
        uint8_t unknown0[2];
        uint8_t count;        // +2
        uint8_t pad3;
        uint16_t index;       // +4 into the id lists
        uint8_t unknown6[4];
    } layers[3];
    uint8_t unknown3E[2];
};
static_assert(sizeof(MusicTrack) == 0x40);

// a transition (0x1E bytes)
struct MusicTransition {
    uint8_t fromCount;        // +0 (0xFF any)
    uint8_t toCount;          // +1 (0xFF any)
    uint8_t unknown2;
    uint8_t stop;             // +3 the search ends at it
    uint8_t unknown4[3];
    uint8_t condA;            // +7 (0x1F any)
    uint8_t condB;            // +8 (0x1F any)
    uint8_t unknown9[3];
    uint16_t fromIndex;       // +0xC
    uint16_t toIndex;         // +0xE
    uint8_t unknown10[9];
    uint8_t enabled19;        // +0x19
    uint8_t kind1;            // +0x1A
    uint8_t kind2;            // +0x1B
    uint8_t kind3;            // +0x1C (also: not back to the same sound)
    uint8_t enabled1D;        // +0x1D
};
static_assert(sizeof(MusicTransition) == 0x1E);

// the combat tracker's snapshot (0x1C bytes)
struct FightInfo {
    int32_t fighters;         // +0
    uint8_t found;            // +4 an attacker in a fighting state
    uint8_t pad5[3];
    CVector pos;              // +8 its target position
    uint8_t playerTargeted;   // +0x14
    uint8_t pad15[3];
    int32_t count;            // +0x18
};
static_assert(sizeof(FightInfo) == 0x1C);

struct FightMusic {
    int32_t thresholds[4];    // +0 (2, 4, 6, 8)
    FightInfo current;        // +0x10
    FightInfo next;           // +0x2C
    uint8_t active;           // +0x48
    uint8_t unknown49[0x23];  // +0x49 (eight dwords from +0x49 zeroed together)
    re::Ptr32<void> sound;              // +0x6C
    int32_t unknown70;        // +0x70
    float unknown74;          // +0x74
    float unknown78;          // +0x78
    int32_t unknown7C;        // +0x7C
    int32_t unknown80;        // +0x80
};
static_assert(offsetof(FightMusic, next) == 0x2C);
static_assert(offsetof(FightMusic, sound) == 0x6C);
static_assert(sizeof(FightMusic) == 0x84);

GAME_VAR(MusicTrack*, tracks, 0x00CF3B04);
GAME_VAR(MusicTransition*, transitions, 0x00CF3B08);
GAME_VAR(int32_t*, soundIds, 0x00CF3B0C);
GAME_VAR(int32_t, numTracks, 0x00CF3AF4);
GAME_VAR(int32_t, numTransitions, 0x00CF3AF8);
GAME_VAR(int32_t, numSoundIds, 0x00CF3B00);
GAME_VAR(int32_t, musicFileUnknown, 0x00CF3AFC);
GAME_ARRAY(int16_t, trackIndex, 0x00CF3AA8);     // [31] by track id (0x1F none)
GAME_ARRAY(uint8_t, musicState, 0x00CF3A30);     // 0x68 bytes of counters (low nibbles)
GAME_ARRAY(uint8_t, musicState2, 0x00CF39C8);    // 0x68 bytes
GAME_VAR(MusicTrack*, currentTrack, 0x00CF3A98);
GAME_VAR(int32_t, musicUnknownA9C, 0x00CF3A9C);
GAME_VAR(uint32_t, nextSwitchTime, 0x00CF3AA0);
GAME_VAR(uint8_t, repeatCount, 0x00CF3AA4);
GAME_VAR(uint32_t, lastCombatTime, 0x00CF3A8C);
GAME_VAR(uint8_t, intensityLevel, 0x00CF3AF0);
GAME_VAR(uint8_t, musicFlagAF2, 0x00CF3AF2);
GAME_VAR(uint8_t, musicStopping, 0x00CF3AF3);
GAME_VAR(int32_t, interactiveMusicOff, 0x00CF3AEC);
GAME_VAR(uint8_t, forcedTrack, 0x00AE6304);
GAME_VAR(int32_t, currentGoKartRace, 0x00AE6284);
GAME_ARRAY(void*, musicChannels, 0x00CF4288);
GAME_VAR(void*, playerPed, 0x00C1AEA8);
GAME_VAR(void*, currentMinigame, 0x020C8704);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(uint32_t, timeMs, 0x00C1A9B4);
GAME_VAR(float, timeStepScale, 0x00C1A9A4);
GAME_VAR(float, fightRumble, 0x00AE6278);
GAME_VAR(float, wantedLevel, 0x00C1AF88);
GAME_VAR(uint8_t, physicalState, 0x00C1AF4C);
GAME_VAR(uint8_t, cutsceneRunning, 0x020C5BE4);
GAME_VAR(uint8_t, flag20C5BE6, 0x020C5BE6);
GAME_VAR(uint8_t, soundsFlagCF3DD5, 0x00CF3DD5);
GAME_VAR(uint8_t, speechPlaying, 0x00CF52DD);
GAME_VAR(void*, speechChannel, 0x00CF4290);
GAME_VAR(void*, radioObject, 0x00CF4374);
// the music streams' state (0xCF42D4 and its crossfade at +0x20)
GAME_VAR(int32_t, musicStreamA, 0x00CF42D4);
GAME_VAR(int32_t, musicStreamAState, 0x00CF42D8);
GAME_VAR(int32_t, musicStreamB, 0x00CF42F4);
GAME_VAR(int32_t, musicStreamBState, 0x00CF42F8);
GAME_VAR(int32_t, musicChannelIndex, 0x00CF4314);
GAME_VAR(uint8_t, musicHasChannel, 0x00CF431C);
GAME_ARRAY(uint8_t, musicFileBuffer, 0x00BD7800);  // [0x1C000]
inline void* const soundBanks = reinterpret_cast<void*>(0x00CF5DC4);
inline void* const missionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const speechQueue = reinterpret_cast<void*>(0x00CF4378);
inline const char* const kFightBank = "FiteGrp.bnk";

GAME_FN(int, __thiscall, SoundBanks_Find, 0x00597FC0, void* banks, const char* name);
GAME_FN(uint8_t, __thiscall, Sound_IsActive, 0x0059CF10, void* sound);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, re::Ptr32<void>* sound, void* value);
GAME_FN(int32_t, __thiscall, Channel_Fn5A6D60, 0x005A6D60, void* channel);
GAME_FN(int32_t, __thiscall, Channel_Fn5A6D80, 0x005A6D80, void* channel);
GAME_FN(int32_t, __thiscall, Channel_Fn5A6DA0, 0x005A6DA0, void* channel);
GAME_FN(void, __thiscall, Channel_Fn8A20A0, 0x008A20A0, void* channel, int);
GAME_FN(void*, __thiscall, Event_GetOffender, 0x004A5CB0, void* event);
GAME_FN(void, __thiscall, Speech_Fn5A4E70, 0x005A4E70, void* queue, void* out, void* ped, int, int, int, int, int, int, int,
        int, uintptr_t);
GAME_FN(void, __thiscall, Fader_Start, 0x0059E5B0, void* fader, float* value, float from, float to, int, int);
inline int __cdecl GameRand() { return crt::Rand(); }
GAME_FN(int, __cdecl, RandomInt, 0x0040DA80, int low, int high);
GAME_FN(int32_t, __thiscall, Vehicle_GetHandlingKind, 0x004CAEE0, void* vehicle);
GAME_FN(uint8_t, __cdecl, IsDayTime, 0x00425FE0);
GAME_FN(uint8_t, __thiscall, MissionMgr_Fn6AC1E0, 0x006AC1E0, void* mgr);
GAME_FN(uint8_t, __thiscall, MissionMgr_Fn6AA7E0, 0x006AA7E0, void* mgr);
GAME_FN(uint8_t, __thiscall, Radio_Fn593F10, 0x00593F10, void* radio);
GAME_FN(int, __cdecl, LoadFile, 0x0042D180, const char* path, void* buffer, int size, const char* mode);
GAME_FN(void*, __cdecl, Malloc, 0x005EEAA0, size_t size);
inline void* __cdecl IM_Memset(void* p, int value, size_t size) { return crt::Memset(p, value, size); }
inline void* __cdecl IM_Memcpy(void* dst, const void* src, size_t size) { return crt::Memcpy(dst, src, size); }
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);

// (the music's own, where modes apply)
GAME_FN(int32_t, __thiscall, Channel_IsIdle, 0x0058E060, void* channel);
GAME_FN(void, __thiscall, FightMusic_Gather, 0x0058E130, FightMusic* self);
GAME_FN(int32_t, __thiscall, Music_PickTransitionSound, 0x0058E520, void* self, re::Ptr32<MusicTransition>* out, int kind, int from, int condA,
        int condB, int layer);
GAME_FN(void, __cdecl, Music_FreeTables, 0x0058ED10);
GAME_FN(uint8_t, __cdecl, Track_SetActive, 0x0058EFB0, MusicTrack* track, int on, float intensity);

MusicTrack* TrackById(int id) {
    int16_t index = trackIndex[id];
    if (index == 0x1F) {
        return nullptr;
    }
    return reinterpret_cast<MusicTrack*>(re::PtrCast<uint8_t*>(tracks) + index * 0x40);
}

float DefaultIntensity() {
    return float(intensityLevel) / 3.0f;
}

// whether one of tracks 10..16 is active
bool OtherTrackActive() {
    for (int id = 10; id <= 16; id++) {
        MusicTrack* t = TrackById(id);
        if (t && t->active) {
            return true;
        }
    }
    return false;
}

int Nibble(int offset) {
    return musicState[offset] & 0xF;
}

// "the combat ended less than 5 seconds ago" gate the fight conditions share
bool CombatGate() {
    if (!(musicState[0x37] & 0xF) && (musicState[0x36] & 0xF)) {
        return timeMs - lastCombatTime <= 5000;
    }
    return true;
}

int RandomIndex(int count) {
    return (GameRand() * (count - 1)) / 0x7FFF;
}

// ---- channels and the combat tracker ---------------------------------------------------

// 0x58E060 (thiscall): a channel with nothing queued, playing or loading
int32_t __fastcall Channel_IsIdleImpl(void* self, re::Edx) {
    if (Channel_Fn5A6D60(self) || Channel_Fn5A6D80(self) || Channel_Fn5A6DA0(self)) {
        return 0;
    }
    return 1;
}

// 0x58E090 (thiscall): eight dwords from +0x49 zeroed
void __fastcall FightMusic_Clear49(FightMusic* self, re::Edx) {
    std::memset(self->unknown49, 0, 0x20);
}

// 0x58E0B0 (thiscall): the constructor
FightMusic* __fastcall FightMusic_Construct(FightMusic* self, re::Edx) {
    self->current.pos = {0.0f, 0.0f, 0.0f};
    self->current.fighters = 0;
    self->current.playerTargeted = 0;
    self->current.found = 0;
    self->current.count = 0;
    self->next.pos = {0.0f, 0.0f, 0.0f};
    self->next.fighters = 0;
    self->next.playerTargeted = 0;
    self->next.found = 0;
    self->next.count = 0;
    self->sound = nullptr;
    self->unknown74 = 0.0f;
    self->unknown78 = 0.0f;
    self->unknown7C = 0;
    self->unknown80 = 0;
    self->active = 0;
    self->thresholds[0] = 2;
    self->thresholds[1] = 4;
    self->thresholds[2] = 6;
    self->thresholds[3] = 8;
    Sound_Set(&self->sound, nullptr);
    self->unknown70 = 0x3C;
    return self;
}

// 0x58E130 (thiscall): the player's attackers counted (the first fighting one's target
// taken, taunts now and then); the player counts as targeted when none fights
void __fastcall FightMusic_GatherImpl(FightMusic* self, re::Edx) {
    uint8_t* player = static_cast<uint8_t*>(playerPed);
    FightInfo info = {};   // (the game leaves the pads after found/playerTargeted uninitialised stack)
    info.pos = {0.0f, 0.0f, 0.0f};
    info.fighters = 0;
    info.playerTargeted = 0;
    info.found = 0;
    info.count = 0;
    if (player && At<int32_t>(player, 0x3C0) > 0) {
        re::Ptr32<void>* list = &At<void*>(player, 0x3C4);
        for (int i = 0; i < At<int32_t>(player, 0x3C0); i++) {
            uint8_t* ped = static_cast<uint8_t*>(list[i]);
            if (!ped || At<int32_t>(ped, 0x136C) == 0) {
                continue;
            }
            int32_t* state = At<int32_t*>(ped, 0x1D30);
            if (state) {
                if (!info.found && (state[0] == 5 || state[0] == 7)) {
                    info.pos = {reinterpret_cast<float*>(state)[3], reinterpret_cast<float*>(state)[4],
                                reinterpret_cast<float*>(state)[5]};
                    info.found = 1;
                }
                if (Event_GetOffender(state) == player) {
                    info.playerTargeted = 1;
                }
            }
            info.fighters++;
            if (At<uint8_t>(ped, 0x1E25)) {
                float rate = fightRumble * timeStepScale;
                int chance = -1 - int(-20.0f / rate);
                if ((GameRand() * chance) / 0x7FFF <= 1) {
                    uint8_t out[0x30];
                    Speech_Fn5A4E70(speechQueue, out, ped, 0x7C, -1, 0, 0, 0, 0, 0, -1, 0);
                }
            }
        }
        if (info.fighters > 0) {
            info.count = info.fighters;
        }
        if (info.found) {
            self->next = info;
            return;
        }
    }
    info.playerTargeted = 1;
    self->next = info;
}

// 0x58E2A0 (thiscall): the combat tracker's update (off during cutscenes; inactive while
// speech or the channel plays)
void __fastcall FightMusic_Update(FightMusic* self, re::Edx) {
    SoundBanks_Find(soundBanks, kFightBank);
    uint8_t flag = soundsFlagCF3DD5;
    if (!cutsceneRunning && !flag20C5BE6 && !flag) {
        FightMusic_Gather(self);
        if (!Channel_IsIdle(speechChannel) && !speechPlaying) {
            self->active = 1;
            self->current = self->next;
            return;
        }
        self->active = 0;
        std::memset(self->unknown49, 0, 0x20);
        self->current = self->next;
        return;
    }
    if (self->sound && !flag) {
        if (Sound_IsActive(self->sound)) {
            reinterpret_cast<void(__thiscall*)(void*)>(static_cast<void*>((*re::PtrCast<re::Ptr32<re::Ptr32<void>>*>(self->sound))[0x34 / 4]))(self->sound);
        }
        Sound_Set(&self->sound, nullptr);
    }
}

// 0x58E360 (thiscall): a fade of the value at +0x20 between two levels
void __fastcall MusicLayer_Fade(uint8_t* self, re::Edx, float from, float to, int a, int b) {
    Fader_Start(self + 0x14, &At<float>(self, 0x20), from, to, a, b);
    self[0x30] = 0;
}

// 0x58E3A0 (thiscall): a fade from the current level (+0x18)
void __fastcall MusicLayer_FadeTo(uint8_t* self, re::Edx, float to, int a, int b) {
    Fader_Start(self + 0x14, &At<float>(self, 0x20), At<float>(self, 0x18), to, a, b);
    self[0x30] = 0;
}

// ---- the transition tables -------------------------------------------------------------

bool IdListed(uint16_t index, int count, int value) {
    for (int i = 0; i < count; i++) {
        if (soundIds[index + i] == value) {
            return true;
        }
    }
    return false;
}

// 0x58E3E0 (stdcall): a random enabled transition from one sound to another under the
// conditions (from any when it lists none)
MusicTransition* __fastcall Music_FindTransition(void*, re::Edx, int from, int to, int condA, int condB) {
    MusicTransition* found[20];
    int count = 0;
    for (int i = 0; i < numTransitions; i++) {
        MusicTransition* t = &transitions[i];
        if (!t->enabled19) {
            continue;
        }
        if (t->condA != 0x1F && t->condA != uint8_t(condA)) {
            continue;
        }
        if (t->condB != 0x1F && t->condB != uint8_t(condB)) {
            continue;
        }
        bool fromOk = true;
        if (t->fromCount) {
            fromOk = IdListed(t->fromIndex, t->fromCount, from);
        }
        if (t->toCount) {
            if (!fromOk || !IdListed(t->toIndex, t->toCount, to)) {
                continue;
            }
        } else if (!fromOk) {
            continue;
        }
        found[count++] = t;
        if (t->stop) {
            break;
        }
    }
    if (count <= 0) {
        return nullptr;
    }
    return found[RandomIndex(count)];
}

// 0x58E520 (stdcall): a random sound to go to from `from` by transitions of a kind, among
// those the track's layer allows (any when it has none); the transition in *out
int32_t __fastcall Music_PickTransitionSoundImpl(void*, re::Edx, re::Ptr32<MusicTransition>* out, int kind, int from, int condA, int condB, int layer) {
    *out = nullptr;
    MusicTrack* track = TrackById(uint8_t(condB));
    MusicTrack::Layer* allowed = reinterpret_cast<MusicTrack::Layer*>(reinterpret_cast<uint8_t*>(track) + 0x20 +
                                                                        uint8_t(layer) * 10);
    struct Pick {
        int32_t id;
        MusicTransition* transition;
    } picks[20];
    int count = 0;
    for (int i = 0; i < numTransitions; i++) {
        MusicTransition* t = &transitions[i];
        if (uint8_t(kind) == 1) {
            if (!t->kind1) {
                continue;
            }
        } else if (uint8_t(kind) == 2) {
            if (!t->kind2) {
                continue;
            }
        } else if (uint8_t(kind) == 3) {
            if (!t->kind3) {
                continue;
            }
        }
        if (!t->toCount) {
            continue;
        }
        if (t->condA != 0x1F && t->condA != uint8_t(condA)) {
            continue;
        }
        if (t->condB != 0x1F && t->condB != uint8_t(condB)) {
            continue;
        }
        if (t->fromCount && !IdListed(t->fromIndex, t->fromCount, from)) {
            continue;
        }
        bool matched = false;
        for (int j = 0; j < t->toCount; j++) {
            uint8_t n = allowed->count;
            if (n > 0) {
                int32_t id = soundIds[t->toIndex + j];
                for (int k = 0; k < n; k++) {
                    int32_t v = soundIds[allowed->index + k];
                    if (v == id && (!t->kind3 || v != from)) {
                        picks[count].id = v;
                        picks[count].transition = t;
                        count++;
                        matched = true;
                    }
                }
            }
            if (n == 0) {
                picks[count].id = soundIds[t->toIndex + j];
                picks[count].transition = t;
                count++;
                matched = true;
            }
        }
        if (matched && t->stop) {
            break;
        }
    }
    if (count <= 0) {
        return 0;
    }
    int index = RandomIndex(count);
    *out = picks[index].transition;
    return picks[index].id;
}

// 0x58E760 (stdcall): a random transition by whether it has from / to lists (0xFF: any)
MusicTransition* __fastcall Music_FindTransition2(void*, re::Edx, int from, int to, int condA, int condB) {
    MusicTransition* found[20];
    int count = 0;
    for (int i = 0; i < numTransitions; i++) {
        MusicTransition* t = &transitions[i];
        if (!t->enabled1D) {
            continue;
        }
        if (to != 0 ? t->toCount == 0 : t->toCount != 0) {
            continue;
        }
        uint8_t fromCount = t->fromCount;
        if (from != 0 ? fromCount == 0 : fromCount != 0) {
            continue;
        }
        if (t->condA != 0x1F && t->condA != uint8_t(condA)) {
            continue;
        }
        if (t->condB != 0x1F && t->condB != uint8_t(condB)) {
            continue;
        }
        if (!(fromCount == 0xFF || (fromCount == 0 && from == 0)) && !IdListed(t->fromIndex, fromCount, from)) {
            continue;
        }
        uint8_t toCount = t->toCount;
        if (!(toCount == 0xFF || (toCount == 0 && to == 0)) && !IdListed(t->toIndex, toCount, to)) {
            continue;
        }
        found[count++] = t;
        if (t->stop) {
            break;
        }
    }
    if (count <= 0) {
        return nullptr;
    }
    return found[RandomIndex(count)];
}

struct ModelVolume {
    uintptr_t model;
    float volume;
};
constexpr ModelVolume kModelVolumes[] = {
    {0x00BF4348, 0.1f}, {0x00BF4344, 0.1f}, {0x00BF4434, 0.1f}, {0x00BF442C, 0.1f}, {0x00BF43DC, 0.1f},
    {0x00BF44E0, 0.1f}, {0x00BF43A8, 0.1f}, {0x00BF4428, 0.25f}, {0x00BF4414, 0.25f}, {0x00BF440C, 0.25f},
    {0x00BF43F0, 0.25f}, {0x00BF43B0, 0.25f}, {0x00BF43EC, 0.25f}, {0x00BF4370, 0.25f}, {0x00BF4158, 0.25f},
    {0x00BF4154, 0.25f}, {0x00BF4150, 0.25f}, {0x00BF414C, 0.25f}, {0x00BF4148, 0.25f}, {0x00BF43A0, 0.25f},
    {0x00BF439C, 0.25f}, {0x00BF4398, 0.25f}, {0x00BF4394, 0.25f}, {0x00BF4390, 0.25f}, {0x00BF444C, 0.5f},
    {0x00BF44D8, 0.5f}, {0x00BF4448, 0.5f}, {0x00BF4444, 0.5f}, {0x00BF443C, 0.5f}, {0x00BF4440, 0.5f},
    {0x00BF4438, 0.5f}, {0x00BF441C, 0.6f}, {0x00BF4418, 0.6f}, {0x00BF43D8, 0.6f}, {0x00BF43D4, 0.6f},
    {0x00BF43D0, 0.6f}, {0x00BF43CC, 0.6f}, {0x00BF4404, 0.6f}, {0x00BF43C4, 0.6f}, {0x00BF4400, 0.6f},
    {0x00BF43C0, 0.6f}, {0x00BF43B8, 0.6f}, {0x00BF4420, 0.75f}, {0x00BF43F8, 0.75f}, {0x00BF437C, 0.75f},
    {0x00BF43AC, 0.75f}, {0x00BF4378, 0.75f}, {0x00BF4408, 0.75f}, {0x00BF436C, 0.75f}, {0x00BF43B4, 0.75f},
    {0x00BF4424, 1.0f}, {0x00BF44D4, 1.0f}, {0x00BF4410, 1.0f}, {0x00BF438C, 1.0f}, {0x00BF4388, 1.0f},
};

// 0x58E8E0 (cdecl): the music's volume for a model index (the model ids are globals,
// compared in this order), 0 for others
float __cdecl Music_ModelVolume(int model) {
    for (const ModelVolume& entry : kModelVolumes) {
        if (model == *reinterpret_cast<const int32_t*>(entry.model)) {
            return entry.volume;
        }
    }
    return 0.0f;
}

// 0x58EBA0 (cdecl): a random sound of the track's current level's layer (else of the
// first layer with any, which becomes the level)
int32_t __cdecl Track_PickSound(MusicTrack* track) {
    uint8_t n = track->layers[track->level].count;
    if (n) {
        int r = RandomIndex(n);
        return soundIds[track->layers[track->level].index + r];
    }
    for (int i = 0; i < 3; i++) {
        if (track->layers[i].count) {
            uint8_t count = track->layers[i].count;
            int r = RandomIndex(count);
            int32_t id = soundIds[track->layers[i].index + r];
            track->level = uint8_t(i);
            return id;
        }
    }
    return 0;
}

// 0x58EC60 (cdecl): a random sound of layer +0x12 other than `avoid` (the next one when
// it's that)
int32_t __cdecl Track_PickOtherSound(MusicTrack* track, int32_t avoid) {
    uint8_t n = track->layers[track->layer12].count;
    int r = RandomIndex(n);
    MusicTrack::Layer& layer = track->layers[track->layer12];
    uint16_t base = layer.index;
    if (soundIds[base + r] == avoid) {
        r++;
        if (r >= layer.count) {
            r = 0;
        }
    }
    return soundIds[base + r];
}

// 0x58ECD0: every track's switch time and activity cleared
void __cdecl Music_ResetTracks() {
    for (int i = 0; i < numTracks; i++) {
        tracks[i].switchTime = 0;
        tracks[i].active = 0;
    }
}

// 0x58ED10: the tables freed and the index emptied
void __cdecl Music_FreeTablesImpl() {
    if (tracks) {
        Free(tracks);
        tracks = nullptr;
    }
    if (transitions) {
        Free(transitions);
        transitions = nullptr;
    }
    if (soundIds) {
        Free(soundIds);
        soundIds = nullptr;
    }
    currentTrack = nullptr;
    musicUnknownA9C = 0;
    uint32_t* index = reinterpret_cast<uint32_t*>(trackIndex);
    for (int i = 0; i < 15; i++) {
        index[i] = 0x1F001F;
    }
    trackIndex[30] = 0x1F;
}

// 0x58EDC0 (thiscall): a stream's state (its crossfade's while it has one)
int32_t __fastcall Stream_GetState(int32_t* self, re::Edx) {
    if (self[8]) {
        return self[9];
    }
    if (self[0]) {
        return self[1];
    }
    return 0;
}

// 0x58EDE0 (thiscall): a stream's +0x14 (its crossfade's +0x34)
int32_t __fastcall Stream_Get14(int32_t* self, re::Edx) {
    if (self[8]) {
        return self[0xD];
    }
    if (self[0]) {
        return self[5];
    }
    return 0;
}

// 0x58EE00 (thiscall): a stream's channel
void* __fastcall Stream_GetChannel(uint8_t* self, re::Edx) {
    if (self[0x48]) {
        return musicChannels[At<int32_t>(self, 0x40)];
    }
    return nullptr;
}

// 0x58EE20 (thiscall): a stream set up on a channel
void __fastcall Stream_Init(uint8_t* self, re::Edx, int channel) {
    At<int32_t>(self, 0) = 0;
    At<int32_t>(self, 0x20) = 0;
    self[0x48] = 1;
    At<int32_t>(self, 0x40) = channel;
    At<int32_t>(self, 0x44) = 0;
    self[0x49] = 0;
}

// 0x58EE40 (thiscall): whether a stream is preloaded (state 2 and its channel ready)
uint8_t __fastcall Stream_IsPreloaded(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x20) != 0 || At<int32_t>(self, 0) != 2) {
        return 0;
    }
    void* channel = self[0x48] ? musicChannels[At<int32_t>(self, 0x40)] : nullptr;
    return Channel_Fn5A6DA0(channel) ? 1 : 0;
}

// 0x58EE70 (stdcall): a kind-3 transition sound (both conditions the same)
int32_t __fastcall Music_PickKind3Sound(void*, re::Edx, re::Ptr32<MusicTransition>* out, int from, int cond, int layer) {
    return Music_PickTransitionSound(nullptr, out, 3, from, cond, cond, layer);
}

// 0x58EE90 (thiscall): the director's current entry's pending stop sent to the music
// channel (returns what the game leaves in eax: the flags' address, else their offset)
uintptr_t __fastcall Director_FlushStop(uint8_t* self, re::Edx) {
    int offset = At<int32_t>(self, 0x90) * 0x30;
    if (!(self[offset + 0x10] & 1)) {
        return uintptr_t(offset);
    }
    void* channel = musicHasChannel ? musicChannels[musicChannelIndex] : nullptr;
    Channel_Fn8A20A0(channel, 1);
    offset = At<int32_t>(self, 0x90) * 0x30;
    self[offset + 0x10] &= 0xFE;
    return reinterpret_cast<uintptr_t>(self + offset + 0x10);
}

// 0x58EEE0 (thiscall): the next switch time and repeat count from the director's
// current entry (its track's when it varies)
void __fastcall Director_SetupTiming(uint8_t* self, re::Edx) {
    int offset = At<int32_t>(self, 0x90) * 0x30;
    uint8_t* entry = At<uint8_t*>(self, offset);
    if (entry[6] == 0) {
        nextSwitchTime = At<uint32_t>(self, offset + 0x28);
        repeatCount = self[offset + 0x2C];
        return;
    }
    if (currentTrack->layers[currentTrack->layer12].count <= 1) {
        return;
    }
    uint16_t delay = At<uint16_t>(entry, 0x14);
    nextSwitchTime = delay ? uint32_t(delay) + timeMs : 0;
    uint8_t* track = At<uint8_t*>(self, At<int32_t>(self, 0x90) * 0x30);
    if (track[0x17] > 0) {
        uint8_t* again = At<uint8_t*>(self, At<int32_t>(self, 0x90) * 0x30);
        repeatCount = uint8_t(RandomInt(again[0x16], again[0x17] + 1));
    } else {
        repeatCount = 0;
    }
}

// 0x58EFB0 (cdecl): a track's condition result applied: its level from the intensity,
// and on/off switched after a random delay; TRUE while it stays active (with a forced
// track: whether it's this one)
uint8_t __cdecl Track_SetActiveImpl(MusicTrack* track, int on, float intensity) {
    uint8_t turnOn = uint8_t(on);
    if (turnOn) {
        track->intensity = intensity;
    }
    float scaled = track->intensity * 3.0f + 0.0001f;
    uint8_t level = uint8_t(int(scaled));
    track->level = level;
    if (level >= 3) {
        track->level = 2;
    }
    if (forcedTrack != 0x1F) {
        track->active = 1;
        return track->id == forcedTrack ? 1 : 0;
    }
    if (!musicStopping && !interactiveMusicOff) {
        if (turnOn != track->active) {
            track->active = turnOn;
            if (track->switchTime > timeMs) {
                track->switchTime = 0;
                return turnOn;
            }
            track->switchTime = timeMs;
            if (turnOn) {
                track->switchTime += RandomInt(track->onDelayMin, track->onDelayMax);
            } else {
                track->switchTime += RandomInt(track->offDelayMin, track->offDelayMax);
            }
            return turnOn == 0;
        }
        if (timeMs >= track->switchTime) {
            return turnOn;
        }
        return turnOn == 0;
    }
    track->active = turnOn;
    track->switchTime = 0;
    return turnOn;
}

// ---- the tracks' conditions (cdecl, each given its track) ------------------------------

bool PlayerInCar() {
    uint8_t* player = static_cast<uint8_t*>(playerPed);
    return player && At<void*>(player, 0x1554) && Vehicle_GetHandlingKind(At<void*>(player, 0x1554)) == 2;
}

// 0x58F0A0: driving a car by day
void __cdecl Condition_CarDay(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = PlayerInCar() && IsDayTime() ? 1 : 0;
    Track_SetActive(track, on, intensity);
}

// 0x58F110: driving a car by night
void __cdecl Condition_CarNight(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = PlayerInCar() && !IsDayTime() ? 1 : 0;
    Track_SetActive(track, on, intensity);
}

// 0x58F180: driving a car in area 0x3E
void __cdecl Condition_CarArea3E(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = PlayerInCar() && currArea == 0x3E ? 1 : 0;
    Track_SetActive(track, on, intensity);
}

// 0x58F1F0: the minigame's +0x38 is 2
void __cdecl Condition_Minigame2(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = currentMinigame && At<int32_t>(currentMinigame, 0x38) == 2 ? 1 : 0;
    Track_SetActive(track, on, intensity);
}

// 0x58F240: in area 0x25
void __cdecl Condition_Area25(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = currArea == 0x25 ? 1 : 0;
    Track_SetActive(track, on, intensity);
}

// 0x58F290: at night with the mission manager's 0x6AC1E0
void __cdecl Condition_NightMission(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = 0;
    if (!IsDayTime() && MissionMgr_Fn6AC1E0(missionMgr)) {
        on = 1;
    }
    Track_SetActive(track, on, intensity);
}

// 0x58F2F0: by day with the mission manager's 0x6AC1E0
void __cdecl Condition_DayMission(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = 0;
    if (IsDayTime() && MissionMgr_Fn6AC1E0(missionMgr)) {
        on = 1;
    }
    Track_SetActive(track, on, intensity);
}

// 0x58F350: go-kart races 0..4
void __cdecl Condition_KartRaces1(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = uint32_t(currentGoKartRace) <= 4 ? 1 : 0;
    Track_SetActive(track, on, intensity);
}

// 0x58F3A0: go-kart races 5..7
void __cdecl Condition_KartRaces2(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = uint32_t(currentGoKartRace - 5) <= 2 ? 1 : 0;
    Track_SetActive(track, on, intensity);
}

// 0x58F3F0: a fight (the combat counters against the others, per track id 0xB..0x10)
void __cdecl Condition_Fight(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = 0;
    if (playerPed) {
        uint8_t id = track->id;
        int slot;
        switch (id) {
        case 0xB:
            slot = 5;
            break;
        case 0xC:
            slot = 2;
            break;
        case 0xD:
            slot = 1;
            break;
        case 0xE:
            slot = 4;
            break;
        case 0xF:
            slot = 6;
            break;
        case 0x10:
            slot = 3;
            break;
        default:
            slot = 0xB;
            break;
        }
        bool noOther = !OtherTrackActive();
        int others = Nibble(0x18) + Nibble(0x2C) + Nibble(0x0C) + Nibble(0x10) + Nibble(0x04) + Nibble(0x08) + Nibble(0x14);
        int fights = Nibble(0x36);
        if (CombatGate()) {
            bool pass;
            if (track->active && fights != 0) {
                pass = true;
            } else if (others == 0 && fights != 0 && noOther && id == 0xA) {
                pass = true;
            } else {
                pass = (musicState[slot * 4 + 2] & 0xF) != 0 && noOther;
            }
            if (pass) {
                int sum = Nibble(0x28) + Nibble(0x24) + Nibble(0x00) + Nibble(0x1C) + Nibble(0x20);
                if (sum < fights) {
                    on = 1;
                    intensity = 0.0f;
                }
            }
        }
    }
    Track_SetActive(track, on, intensity);
}

// 0x58F680: one of the player's list has +0x1398 set
void __cdecl Condition_Chased(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = 0;
    uint8_t* player = static_cast<uint8_t*>(playerPed);
    if (player) {
        re::Ptr32<void>* list = &At<void*>(player, 0x3C4);
        for (int i = 0; i < At<int32_t>(player, 0x3C0); i++) {
            if (list[i] && At<int32_t>(list[i], 0x1398) != 0) {
                on = 1;
                break;
            }
        }
    }
    Track_SetActive(track, on, intensity);
}

// 0x58F700: the player's +0x1E91
void __cdecl Condition_Player1E91(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = playerPed && At<uint8_t>(playerPed, 0x1E91) ? 1 : 0;
    Track_SetActive(track, on, intensity);
}

// 0x58F760: minigame 10 (at intensity 0)
void __cdecl Condition_Minigame10(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = 0;
    if (playerPed && currentMinigame && At<int32_t>(currentMinigame, 0x3C) == 10) {
        intensity = 0.0f;
        on = 1;
    }
    Track_SetActive(track, on, intensity);
}

// 0x58F7C0: the player's +0x131C set (by night full; by day 0 or 0.4 by +0x1D40)
void __cdecl Condition_Player131C(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = 0;
    uint8_t* player = static_cast<uint8_t*>(playerPed);
    if (player && At<int32_t>(player, 0x131C) != 0) {
        if (!IsDayTime()) {
            intensity = 1.0f;
        } else if (At<int32_t>(player, 0x1D40) < 200) {
            intensity = 0.0f;
        } else {
            intensity = 0.4f;
        }
        on = 1;
    }
    Track_SetActive(track, on, intensity);
}

// 0x58F840: the chase counters (and the track itself while active)
void __cdecl Condition_Chase(MusicTrack* track) {
    float intensity = DefaultIntensity();
    track->unknown1D = 0;
    int on = 0;
    if ((playerPed && (musicState[0x36] & 0xF)) || track->active) {
        bool noOther = !OtherTrackActive();
        int n1C = Nibble(0x1C);
        int n00 = Nibble(0x00);
        int sum = Nibble(0x24) + n1C + Nibble(0x28) + Nibble(0x20) + n00;
        bool gate = true;
        if (!(musicState[0x37] & 0xF) && (musicState[0x36] & 0xF) && timeMs - lastCombatTime > 5000) {
            gate = false;
        }
        if (gate && (sum > 0 || (track->active && !noOther && !musicStopping))) {
            on = 1;
            if (track->active) {
                intensity = track->intensity;
            } else if (n00) {
                intensity = 1.0f;
            } else if (n1C) {
                intensity = 0.5f;
            } else {
                intensity = 0.0f;
            }
        }
    }
    Track_SetActive(track, on, intensity);
}

// 0x58F9A0: never
void __cdecl Condition_Never(MusicTrack* track) {
    float intensity = DefaultIntensity();
    Track_SetActive(track, 0, intensity);
}

// 0x58F9E0: never (the time of day read and dropped)
void __cdecl Condition_NeverDay(MusicTrack* track) {
    float intensity = DefaultIntensity();
    IsDayTime();
    Track_SetActive(track, 0, intensity);
}

// 0x58FA20: wanted (the radio full, else 0.4 when the player's +0x1D0 has a model)
void __cdecl Condition_Wanted(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = 0;
    uint8_t* player = static_cast<uint8_t*>(playerPed);
    if (player && double(wantedLevel) > 0.10000000149011612) {
        on = 1;
        if (Radio_Fn593F10(radioObject)) {
            intensity = 1.0f;
        } else if (At<int16_t>(At<void*>(player, 0x1D0), 0x10E) != -1) {
            intensity = 0.4f;
        } else {
            intensity = 0.0f;
        }
    }
    Track_SetActive(track, on, intensity);
}

// 0x58FAC0: flag 0xCF3AF2 (the intensity from track 0x1B's layers)
void __cdecl Condition_Track1B(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = 0;
    MusicTrack* other = TrackById(0x1B);
    if (other) {
        on = musicFlagAF2;
        if (musicFlagAF2) {
            bool noOther = !OtherTrackActive();
            bool inCar = PlayerInCar();
            on = musicFlagAF2;
            if (soundIds[other->layers[2].index] != 0 && !noOther) {
                intensity = 1.0f;
            } else if (soundIds[other->layers[1].index] != 0 && (!noOther || inCar)) {
                intensity = 0.5f;
            } else {
                intensity = 0.0f;
            }
        }
    }
    Track_SetActive(track, on, intensity);
}

// 0x58FBE0: physical state 2 outside missions (at intensity 0)
void __cdecl Condition_PhysicalState2(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = 0;
    if (playerPed && !MissionMgr_Fn6AA7E0(missionMgr) && physicalState == 2) {
        intensity = 0.0f;
        on = 1;
    }
    Track_SetActive(track, on, intensity);
}

// 0x58FC50: outdoors outside minigames (not in some areas), louder in a car
void __cdecl Condition_Ambient(MusicTrack* track) {
    float intensity = DefaultIntensity();
    int on = 1;
    if (currentMinigame) {
        on = 0;
    } else {
        switch (currArea) {
        case 8:
        case 16:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 33:
        case 34:
        case 39:
        case 46:
        case 50:
        case 55:
        case 56:
        case 57:
        case 59:
        case 60:
        case 61:
            on = 0;
            break;
        }
        intensity = 0.0f;
        uint8_t* player = static_cast<uint8_t*>(playerPed);
        if (player) {
            void* vehicle = At<void*>(player, 0x1554);
            if (vehicle && Vehicle_GetHandlingKind(vehicle) == 2) {
                intensity = 1.0f;
            } else if (At<int32_t>(playerPed, 0x148C) != 0) {
                intensity = 0.5f;
            }
        }
    }
    Track_SetActive(track, on, intensity);
}

// 0x58FD40: whether music is audible (track 0x1E needs its channel's volume)
uint8_t __cdecl Music_IsAudible() {
    MusicTrack* track = currentTrack;
    if (!track) {
        return 0;
    }
    if (track->id != 0x1E) {
        return 1;
    }
    void* channel = musicHasChannel ? musicChannels[musicChannelIndex] : nullptr;
    return At<float>(channel, 0x18) < 0.0001f ? 0 : 1;
}

// 0x58FD80: the music told to stop; TRUE while its stream is still in state 5
uint8_t __cdecl Music_Stop() {
    musicStopping = 1;
    if (!currentTrack) {
        return 0;
    }
    int32_t state;
    if (musicStreamB) {
        state = musicStreamBState;
    } else if (musicStreamA) {
        state = musicStreamAState;
    } else {
        return 0;
    }
    return state == 5 ? 1 : 0;
}

void* NewArray(uint32_t count, uint32_t size) {
    uint64_t bytes = uint64_t(count) * size;
    return Malloc((bytes >> 32) ? size_t(0xFFFFFFFF) : size_t(bytes));
}

// 0x58FDC0: audio\config\IMusic.bin read: its tracks, transitions and sound ids copied out
// (after a 16-byte header of their counts) and the tracks indexed by id
void __cdecl Music_Load() {
    IM_Memset(musicState, 0, 0x68);
    IM_Memset(musicState2, 0, 0x68);
    Music_FreeTables();
    LoadFile("audio\\config\\IMusic.bin", musicFileBuffer, 0x1C000, "r");
    const int32_t* header = reinterpret_cast<const int32_t*>(musicFileBuffer);
    musicFileUnknown = header[2];
    numTracks = header[0];
    numTransitions = header[1];
    numSoundIds = header[3];
    int offset = 0x10;
    if (numTracks) {
        tracks = static_cast<MusicTrack*>(NewArray(uint32_t(numTracks), 0x40));
        IM_Memcpy(tracks, musicFileBuffer + 0x10, size_t(numTracks) << 6);
        offset = numTracks * 0x40 + 0x10;
    }
    if (numTransitions) {
        transitions = static_cast<MusicTransition*>(NewArray(uint32_t(numTransitions), 0x1E));
        IM_Memcpy(transitions, musicFileBuffer + offset, size_t(numTransitions) * 0x1E);
        offset += numTransitions * 0x1E;
    }
    if (numSoundIds) {
        soundIds = static_cast<int32_t*>(NewArray(uint32_t(numSoundIds), 4));
        IM_Memcpy(soundIds, musicFileBuffer + offset, size_t(numSoundIds) * 4);
    }
    for (int i = 0; i < numTracks; i++) {
        trackIndex[tracks[i].id] = int16_t(i);
    }
}

} // namespace

#define IMUSIC_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(imusic_original_, __LINE__);                                       \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(imusic_original_, __LINE__), \
             re::Mode::Original)

IMUSIC_REVERSED("SoundChannel::IsIdle", 0x0058E060, &Channel_IsIdleImpl);
IMUSIC_REVERSED("FightMusic::Clear49", 0x0058E090, &FightMusic_Clear49);
IMUSIC_REVERSED("FightMusic::FightMusic", 0x0058E0B0, &FightMusic_Construct);
IMUSIC_REVERSED("FightMusic::Gather", 0x0058E130, &FightMusic_GatherImpl);
IMUSIC_REVERSED("FightMusic::Update", 0x0058E2A0, &FightMusic_Update);
IMUSIC_REVERSED("MusicLayer::Fade", 0x0058E360, &MusicLayer_Fade);
IMUSIC_REVERSED("MusicLayer::FadeTo", 0x0058E3A0, &MusicLayer_FadeTo);
IMUSIC_REVERSED("InteractiveMusic::FindTransition", 0x0058E3E0, &Music_FindTransition);
IMUSIC_REVERSED("InteractiveMusic::PickTransitionSound", 0x0058E520, &Music_PickTransitionSoundImpl);
IMUSIC_REVERSED("InteractiveMusic::FindTransition2", 0x0058E760, &Music_FindTransition2);
IMUSIC_REVERSED("InteractiveMusic::ModelVolume", 0x0058E8E0, &Music_ModelVolume); // callers rely on edx: InteractiveMusic::GatherStats 0x590C90 (MusicStream.cpp; turn on together)
IMUSIC_REVERSED("MusicTrack::PickSound", 0x0058EBA0, &Track_PickSound);
IMUSIC_REVERSED("MusicTrack::PickOtherSound", 0x0058EC60, &Track_PickOtherSound);
IMUSIC_REVERSED("InteractiveMusic::ResetTracks", 0x0058ECD0, &Music_ResetTracks);
IMUSIC_REVERSED("InteractiveMusic::FreeTables", 0x0058ED10, &Music_FreeTablesImpl);
IMUSIC_REVERSED("MusicStream::GetState", 0x0058EDC0, &Stream_GetState);
IMUSIC_REVERSED("MusicStream::Get14", 0x0058EDE0, &Stream_Get14);
IMUSIC_REVERSED("MusicStream::GetChannel", 0x0058EE00, &Stream_GetChannel); // callers rely on edx: InteractiveMusic::Update 0x591B00 (MusicStream.cpp; turn on together)
IMUSIC_REVERSED("MusicStream::Init", 0x0058EE20, &Stream_Init);
IMUSIC_REVERSED("MusicStream::IsPreloaded", 0x0058EE40, &Stream_IsPreloaded);
IMUSIC_REVERSED("InteractiveMusic::PickKind3Sound", 0x0058EE70, &Music_PickKind3Sound);
IMUSIC_REVERSED("MusicDirector::FlushStop", 0x0058EE90, &Director_FlushStop);
IMUSIC_REVERSED("MusicDirector::SetupTiming", 0x0058EEE0, &Director_SetupTiming);
IMUSIC_REVERSED("MusicTrack::SetActive", 0x0058EFB0, &Track_SetActiveImpl);
IMUSIC_REVERSED("MusicCondition::CarDay", 0x0058F0A0, &Condition_CarDay);
IMUSIC_REVERSED("MusicCondition::CarNight", 0x0058F110, &Condition_CarNight);
IMUSIC_REVERSED("MusicCondition::CarArea3E", 0x0058F180, &Condition_CarArea3E);
IMUSIC_REVERSED("MusicCondition::Minigame2", 0x0058F1F0, &Condition_Minigame2);
IMUSIC_REVERSED("MusicCondition::Area25", 0x0058F240, &Condition_Area25);
IMUSIC_REVERSED("MusicCondition::NightMission", 0x0058F290, &Condition_NightMission);
IMUSIC_REVERSED("MusicCondition::DayMission", 0x0058F2F0, &Condition_DayMission);
IMUSIC_REVERSED("MusicCondition::KartRaces1", 0x0058F350, &Condition_KartRaces1);
IMUSIC_REVERSED("MusicCondition::KartRaces2", 0x0058F3A0, &Condition_KartRaces2);
IMUSIC_REVERSED("MusicCondition::Fight", 0x0058F3F0, &Condition_Fight);
IMUSIC_REVERSED("MusicCondition::Chased", 0x0058F680, &Condition_Chased);
IMUSIC_REVERSED("MusicCondition::Player1E91", 0x0058F700, &Condition_Player1E91);
IMUSIC_REVERSED("MusicCondition::Minigame10", 0x0058F760, &Condition_Minigame10);
IMUSIC_REVERSED("MusicCondition::Player131C", 0x0058F7C0, &Condition_Player131C);
IMUSIC_REVERSED("MusicCondition::Chase", 0x0058F840, &Condition_Chase);
IMUSIC_REVERSED("MusicCondition::Never", 0x0058F9A0, &Condition_Never);
IMUSIC_REVERSED("MusicCondition::NeverDay", 0x0058F9E0, &Condition_NeverDay);
IMUSIC_REVERSED("MusicCondition::Wanted", 0x0058FA20, &Condition_Wanted);
IMUSIC_REVERSED("MusicCondition::Track1B", 0x0058FAC0, &Condition_Track1B);
IMUSIC_REVERSED("MusicCondition::PhysicalState2", 0x0058FBE0, &Condition_PhysicalState2);
IMUSIC_REVERSED("MusicCondition::Ambient", 0x0058FC50, &Condition_Ambient);
IMUSIC_REVERSED("InteractiveMusic::IsAudible", 0x0058FD40, &Music_IsAudible);
IMUSIC_REVERSED("InteractiveMusic::Stop", 0x0058FD80, &Music_Stop);
IMUSIC_REVERSED("InteractiveMusic::Load", 0x0058FDC0, &Music_Load);
