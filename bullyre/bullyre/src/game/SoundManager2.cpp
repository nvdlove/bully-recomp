// The audio manager's update (TheSounds, 0xCF3DA0; see SoundManager.cpp): the vehicles'
// and rats' sounds by distance, the school's and the town's clock towers and the
// heartbeat, the loop slots' playing helpers and the skateboard. 0x596530..0x5978F0.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "game/Pool.h"
#include "game/VCall.h"
#include "reversed.h"

namespace {

using re::At;

struct CVector {
    float x, y, z;
};

// what the SFX manager's play takes (0x24 bytes)
struct SoundParams {
    int32_t volume;       // +0
    int32_t unknown4;     // +4 (the game's uninitialised stack: zero here)
    CVector pos;          // +8
    uint8_t unknown14;    // +0x14
    uint8_t pad15[3];     // (uninitialised)
    int32_t unknown18;    // +0x18 (uninitialised)
    int32_t at;           // +0x1C
    uint32_t flags;       // +0x20
};
static_assert(sizeof(SoundParams) == 0x24);

// a loop slot (0xCF3C08, 48 of them)
struct LoopSlot {
    re::Ptr32<uint8_t> sound;
    uint8_t keep;
    uint8_t pad[3];
};

// a sound's voices (a vector: begin, end, count)
struct VoiceList {
    re::Ptr32<int32_t> voices;
    int32_t unknown4;
    uint32_t count;
};

// the skateboard's sounds
struct SkateAudio {
    re::Ptr32<uint8_t> ped;        // +0 the skater
    int32_t unknown4;    // +4 (3)
    re::Ptr32<uint8_t> roll;       // +8 a sound (Sound_Set)
    re::Ptr32<uint8_t> skid;       // +0xC a sound (the turn's or the skid's)
    float lean;          // +0x10 (smoothed)
    float speed;         // +0x14 (smoothed)
};
static_assert(sizeof(SkateAudio) == 0x18);

inline uint8_t* const theSounds = reinterpret_cast<uint8_t*>(0x00CF3DA0);
inline uint8_t* const streamChannels = reinterpret_cast<uint8_t*>(0x00CF4284);
inline uint8_t* const soundBanks = reinterpret_cast<uint8_t*>(0x00CF5DC4);
inline void* const voiceEngine = reinterpret_cast<void*>(0x00CF7268);
inline void* const missionMgr = reinterpret_cast<void*>(0x020C3CA0);
inline void* const theZones = reinterpret_cast<void*>(0x020C7E98);
inline void* const streamEmitter = reinterpret_cast<void*>(0x00AE5F60);
inline void* const refCF3D88 = reinterpret_cast<void*>(0x00CF3D88);
GAME_ARRAY(LoopSlot, loopSlots, 0x00CF3C08);
GAME_VAR(void*, sfxManager, 0x00CF68CC);
GAME_VAR(uint8_t*, ambience, 0x00CF4374);
GAME_VAR(CPool<uint8_t>*, pedPool, 0x00C0F5F0);
GAME_VAR(CPool<uint8_t>*, vehiclePool, 0x00C0F5F4);
GAME_VAR(CPool<uint8_t>*, triggerPool, 0x020C7C48);
GAME_VAR(uint8_t*, playerPed, 0x00C1AEA8);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(int32_t, ratModel, 0x00BF42DC);
GAME_VAR(uint16_t, vehicleSoundRange, 0x00AE6F06);
GAME_VAR(uint32_t, frameCounter, 0x00C1A99C);
GAME_VAR(uint8_t, clockHours, 0x00BCEB4B);
GAME_VAR(uint8_t, clockMinutes, 0x00BCEB4A);
GAME_VAR(float, soundTimeStep, 0x00CF60F0);
GAME_VAR(float, timeStepScale, 0x00AE6B7C);
GAME_VAR(float, ms_timeStep, 0x00C1A9A4);
GAME_VAR(uint32_t, m_snTimeInMilliseconds, 0x00C1A9B4);
GAME_VAR(uint8_t, userStart, 0x020C32E9);
GAME_VAR(uint8_t, flag20C32EA, 0x020C32EA);
GAME_VAR(uint8_t, pausedC1A999, 0x00C1A999);
GAME_VAR(uint8_t, pausedC1A998, 0x00C1A998);
GAME_VAR(int32_t, refCF3D88Frames, 0x00CF3D8C);

GAME_FN(CVector*, __cdecl, GetCameraPos, 0x00595630, CVector* out);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, void* vehicle, int32_t* out);
GAME_FN(uint32_t*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* key, const char* name);
GAME_FN(int32_t, __thiscall, MissionMgr_FindMission, 0x006A9FD0, void* mgr, uint32_t* key);
GAME_FN(uint8_t, __thiscall, MissionMgr_IsRunning, 0x006AA940, void* mgr, int32_t mission);
GAME_FN(void, __thiscall, VehicleAudios_Remove, 0x0058C430, void* audios, void* vehicle);
GAME_FN(void, __thiscall, VehicleAudios_Add, 0x0058C4B0, void* audios, void* vehicle);
GAME_FN(void, __thiscall, BikeAudios_Remove, 0x0058DF20, void* audios, void* vehicle);
GAME_FN(void, __thiscall, BikeAudios_Add, 0x0058E020, void* audios, void* vehicle);
GAME_FN(int, __thiscall, SoundBanks_Find, 0x00597FC0, void* banks, const char* name);
GAME_FN(void, __thiscall, SoundBanks_Request, 0x0059A860, void* banks, int bank, int time, int a);
GAME_FN(uint8_t, __thiscall, SoundBanks_IsLoaded, 0x005995F0, void* banks, int bank);
GAME_FN(void, __thiscall, SoundBanks_Load, 0x0059A3E0, void* banks, int bank, int, int);
GAME_FN(void, __thiscall, SoundBanks_Unload, 0x0059A420, void* banks, int bank, int, int);
GAME_FN(void, __thiscall, SoundBanks_Fn59A7D0, 0x0059A7D0, void* banks, int bank);
GAME_FN(void, __thiscall, SoundBanks_Update, 0x0059AF90, void* banks);
GAME_FN(void, __thiscall, RatSounds_Add, 0x00593660, void* list, void* rat);
GAME_FN(void, __thiscall, RatSounds_Remove, 0x005935A0, void* list, void* rat);
GAME_FN(int32_t, __thiscall, Zones_FindByName, 0x006D7A10, void* zones, const char* name);
GAME_FN(uint8_t, __thiscall, Zone_IsPointInside, 0x006D6EB0, void* zone, const float* xy);
GAME_FN(uint8_t, __cdecl, IsTimeInRange, 0x00425E40, int fromHours, int toHours, int fromMinutes, int toMinutes);
GAME_FN(uint8_t, __cdecl, IsPlayerInFastVehicle, 0x004469A0);
GAME_FN(int32_t, __cdecl, HashStreamName, 0x005A7D60, const char* name, int arg);
GAME_FN(void*, __thiscall, Sounds_PlayAt, 0x0059B8A0, void* sounds, int id, const CVector* pos, int a3,
        const CVector* pos2, int a5, int a6, int a7, int a8, void* callback, void* entity, int a11, int a12,
        float a13, int a14);
GAME_FN(int32_t, __thiscall, Sounds_Fn59B740, 0x0059B740, void* sounds, const CVector* pos, uintptr_t);
GAME_FN(void*, __thiscall, Sfx_Play2, 0x0059E0F0, void* sfx, int id, SoundParams* params, int, int, uintptr_t, int, int,
        int, int, void* callback, uintptr_t, int, int);
GAME_FN(void, __thiscall, Sound_Set, 0x0059D110, void* soundRef, void* sound);
GAME_FN(void, __thiscall, Sound_GetVoices, 0x0059CF20, void* sound, VoiceList* out);
GAME_FN(int32_t, __thiscall, SfxItem_Fn59D440, 0x0059D440, void* sound, float value);
GAME_FN(uint8_t, __cdecl, Voice_Fn5AF530, 0x005AF530, int voice);
GAME_FN(void, __cdecl, Voice_Fn5AF870, 0x005AF870, int voice, int volume, float);
GAME_FN(void, __cdecl, Voice_Fn5AF7B0, 0x005AF7B0, int voice, int volume, int, int);
GAME_FN(void, __cdecl, Voice_Fn5AF660, 0x005AF660, int voice, float);
GAME_FN(float, __cdecl, Audio_Fn5A7F00, 0x005A7F00, float, float);
GAME_FN(float, __cdecl, Audio_Pitch5A7EA0, 0x005A7EA0, int);
GAME_FN(float, __cdecl, Audio_Fn59E4F0, 0x0059E4F0, float value, int);
GAME_FN(uint8_t, __cdecl, Audio_ApproachF, 0x0059CFC0, float* value, float target, float step);
GAME_FN(void, __cdecl, Free, 0x005EEAB0, void* p);
GAME_FN(uint32_t, __cdecl, Timer_GetCurrentTime, 0x0045BCA0);
GAME_FN(uint8_t, __cdecl, IsInTitleMenu, 0x00404F40);
GAME_FN(void, __cdecl, Voices_Pause, 0x005AF9B0);
GAME_FN(void, __cdecl, Voices_Resume, 0x005AF9E0);
GAME_FN(void, __thiscall, StreamChannels_Pause, 0x005A60D0, void* channels);
GAME_FN(void, __thiscall, StreamChannels_Resume, 0x005A6100, void* channels);
GAME_FN(void, __thiscall, StreamChannels_Update, 0x005A64D0, void* channels);
GAME_FN(void, __thiscall, VoiceEngine_Pause, 0x005A96D0, void* engine, int on);
GAME_FN(void, __thiscall, VoiceEngine_Update, 0x005AA710, void* engine);
GAME_FN(void, __cdecl, Audio_Fn5A8850, 0x005A8850);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, void* ref, void* entity);
GAME_FN(void, __thiscall, Environment_Update, 0x00593AC0, void* environment);
GAME_FN(void, __thiscall, FireAlarm_Update, 0x005959B0, void* alarm);
GAME_FN(void, __thiscall, Ambience_Update, 0x00594B00, void* ambience);
GAME_FN(void, __thiscall, SfxManager_Update, 0x0059DE00, void* sfx);
GAME_FN(void, __thiscall, BikeAudios_Update, 0x0058DFA0, void* audios);
GAME_FN(void, __thiscall, VehicleAudios_Update, 0x0058C360, void* audios);
GAME_FN(void, __thiscall, MusicStream_Update, 0x00591520, void* stream);
GAME_FN(void, __thiscall, AmbienceEmitters_Update, 0x00588030, void* emitters);
GAME_FN(void, __thiscall, StreamEmitter_Update, 0x00587800, void* emitter);
GAME_FN(void, __thiscall, LoopSounds_Update, 0x00592880, void* loops);
GAME_FN(void, __thiscall, RatSounds_Update, 0x00593620, void* rats);
GAME_FN(void, __thiscall, SpeechLib_Update, 0x0059F9D0, void* lib);
GAME_FN(void, __thiscall, Speech_Update5A5B30, 0x005A5B30, void* speech);
GAME_FN(void, __thiscall, FightMusic_Update, 0x0058E2A0, void* music);
GAME_FN(void, __thiscall, Obj58D870_Update, 0x0058DB40, void* self);
GAME_FN(void, __cdecl, InteractiveMusic_Update, 0x00591B00);
GAME_FN(void, __cdecl, Voices_UpdateTime, 0x005AFA80, uint32_t time);
GAME_FN(void, __thiscall, Sounds_UpdateLoopSlots, 0x00594DC0, void* self);
GAME_FN(float, __cdecl, Pad_GetValue, 0x00738840, int control, int pad);
GAME_FN(float, __thiscall, Vector_Magnitude2D, 0x004246B0, const CVector* v);
GAME_FN(CVector*, __thiscall, Placeable_GetForward, 0x004154A0, void* placeable, CVector* out);
GAME_FN(float, __thiscall, Vector_Normalise, 0x00414E10, CVector* v);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);
// its own
GAME_FN(void, __thiscall, Sounds_UpdateVehicles, 0x00596530, void* self);
GAME_FN(void, __thiscall, Sounds_UpdateRats, 0x005969D0, void* self);
GAME_FN(void, __thiscall, Sounds_UpdateClockTowers, 0x00596B00, void* flags);
GAME_FN(void, __cdecl, Sounds_PlayLoopAt, 0x00597010, re::Ptr32<uint8_t>* ref, int32_t id, const CVector* pos, int a4,
        int volume, int pitch, int a7, int flag);
GAME_FN(void, __thiscall, SkateAudio_UpdateFn, 0x005978F0, void* self);
GAME_FN(void, __thiscall, SkateAudio_SkateSounds, 0x00595390, void* self, int32_t* roll, int32_t* turnSound,
        float turn);

const CVector* PositionOf(uint8_t* entity) {
    uint8_t* matrix = At<uint8_t*>(entity, 0x14);
    return reinterpret_cast<const CVector*>(matrix ? matrix + 0x30 : entity + 4);
}

uint8_t MissionRunning(const char* name) {
    uint32_t key;
    int32_t mission = MissionMgr_FindMission(missionMgr, NameKey_Construct(&key, name));
    return MissionMgr_IsRunning(missionMgr, mission);
}

// the distance from the listener (the game's x87 order: y and x squared, then z)
int32_t ListenerDistance(const CVector* pos, const CVector* listener) {
    CVector delta;
    VectorSub(&delta, pos, listener);
    float sq = std::fabs(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    return int32_t(std::sqrt(sq));
}

int LoopSlotOf(uint8_t* sound) {
    for (int i = 0; i < 48; i++) {
        if (loopSlots[i].sound == sound) {
            return i;
        }
    }
    return -1;
}

int FreeLoopSlot() {
    for (int i = 0; i < 48; i++) {
        if (loopSlots[i].sound == nullptr) {
            return i;
        }
    }
    return -1;
}

// the voices of a sound at a volume and a pitch; `arg` null: the sound's vf 0xB8 per voice
void SetVoices(uint8_t* sound, int volume, float pitch, const int* arg) {
    VoiceList list{nullptr, 0, 0};
    Sound_GetVoices(sound, &list);
    for (uint32_t i = 0; i < list.count; i++) {
        if (list.voices[i] == -1) {
            continue;
        }
        int a = arg != nullptr ? *arg : vc::Call<int>(sound, 0xB8);
        if (Voice_Fn5AF530(list.voices[i]) != 0) {
            Voice_Fn5AF870(list.voices[i], volume, 0.0f);
        } else {
            Voice_Fn5AF7B0(list.voices[i], volume, a, 1);
        }
        Voice_Fn5AF660(list.voices[i], pitch);
    }
    if (list.voices != nullptr) {
        Free(list.voices);
    }
}

// ---- the vehicles and the rats ------------------------------------------------------------

// 0x596530 (thiscall): the vehicles near the listener get their sounds (those far away
// lose them), and the banks the near ones need are requested
void __fastcall Sounds_UpdateVehiclesImpl(uint8_t* self) {
    CPool<uint8_t>* pool = vehiclePool;
    uint8_t carNear = 0;
    uint8_t bikeNear = 0;
    uint8_t bikeMission = 0;
    CVector listener;
    GetCameraPos(&listener);
    int slot = pool->size;
    if (slot == 0) {
        return;
    }
    do {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* vehicle = pool->objects + pool->elemSize * slot;
        if (vehicle == nullptr) {
            continue;
        }
        CVector pos = *PositionOf(vehicle);
        int32_t distance = ListenerDistance(&pos, &listener);
        int32_t out;
        int32_t cls = *Vehicle_GetVehicleClass(vehicle, &out);
        switch (cls) {
        case 1: case 3: case 4: case 5:
            if (distance < 100) {
                carNear = 1;
            }
            break;
        case 2: case 9: case 0xB: case 0xC: case 0xD: case 0xE: {
            uint8_t* bike = ((vehicle[0x108] & 7) == 2 && At<int32_t>(vehicle, 0x46C) == 1) ? vehicle : nullptr;
            bikeNear = 1;
            uint8_t mission;
            if (bike != nullptr && At<uint8_t*>(bike, 0x6BC) == playerPed) {
                mission = 1;
            } else {
                mission = MissionRunning("3_04") != 0 || MissionRunning("3_B") != 0;
            }
            if (mission) {
                bikeMission = 1;
            } else if (distance > 0x32) {
                bikeNear = 0;
            }
            if (MissionRunning("6_B") != 0) {
                bikeNear = 0;
            }
            break;
        }
        default:
            break;
        }
        bool isVehicle = (vehicle[0x108] & 7) == 2;
        if ((distance < int32_t(vehicleSoundRange) || cls == 10) &&
            ((vehicle[0x33A] & 2) != 0 || (vehicle[0x33C] & 0x20) != 0)) {
            switch (cls) {
            case 1: case 3: case 4: case 5: case 6: case 7: case 0xA:
                if (isVehicle) {
                    VehicleAudios_Add(At<void*>(self, 0x2B34), vehicle);
                }
                break;
            case 8:
                if (isVehicle && At<int32_t>(vehicle, 0x358) != 0) {
                    VehicleAudios_Add(At<void*>(self, 0x2B34), vehicle);
                }
                break;
            case 2: case 9: case 0xB: case 0xC: case 0xD: case 0xE:
                if (isVehicle && At<int32_t>(vehicle, 0x46C) == 1) {
                    BikeAudios_Add(At<void*>(self, 0x2B30), vehicle);
                }
                break;
            default:
                break;
            }
        } else {
            switch (cls) {
            case 1: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
                if (isVehicle) {
                    VehicleAudios_Remove(At<void*>(self, 0x2B34), vehicle);
                }
                break;
            case 2: case 0xB: case 0xC: case 0xD: case 0xE:
                if (isVehicle && At<int32_t>(vehicle, 0x46C) == 1) {
                    BikeAudios_Remove(At<void*>(self, 0x2B30), vehicle);
                }
                break;
            default:
                break;
            }
        }
    } while (slot != 0);
    if (carNear) {
        SoundBanks_Request(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\CarDamge.bnk"), 5000, 0);
        SoundBanks_Request(soundBanks, SoundBanks_Find(soundBanks, "Engine.bnk"), 5000, 0);
    }
    if (bikeNear) {
        if (bikeMission) {
            if (currArea == 0x3E) {
                SoundBanks_Request(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBMX.bnk"), 5000, 0);
            } else {
                SoundBanks_Request(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkA.bnk"), 5000, 0);
                SoundBanks_Request(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkB.bnk"), 5000, 0);
            }
        } else {
            SoundBanks_Request(soundBanks, SoundBanks_Find(soundBanks, "BikeBank2.bnk"), 2000, 0);
        }
    }
}

// 0x5969D0 (thiscall): the rats within 10 m squeak
void __fastcall Sounds_UpdateRatsImpl(uint8_t* self) {
    CPool<uint8_t>* pool = pedPool;
    CVector listener;
    GetCameraPos(&listener);
    for (int slot = pool->size; slot != 0;) {
        slot--;
        if (pool->flags[slot] & 0x80) {
            continue;
        }
        uint8_t* ped = pool->objects + pool->elemSize * slot;
        if (ped == nullptr || int32_t(At<int16_t>(ped, 0x10E)) != ratModel) {
            continue;
        }
        CVector pos = *PositionOf(ped);
        int32_t distance = ListenerDistance(&pos, &listener);
        if ((ped[0x108] & 7) != 3) {
            continue;
        }
        if (distance < 10) {
            RatSounds_Add(At<void*>(self, 0x4DC), ped);
        } else {
            RatSounds_Remove(At<void*>(self, 0x4DC), ped);
        }
    }
}

// ---- the clock towers and the heartbeat -------------------------------------------------

// a trigger zone holding the player (by the zone's name)
bool PlayerInZone(const char* name) {
    int32_t index = Zones_FindByName(theZones, name);
    if (index == -1 || (triggerPool->flags[index] & 0x80) != 0) {
        return false;
    }
    uint8_t* zone = triggerPool->objects + triggerPool->elemSize * index;
    if (zone == nullptr || playerPed == nullptr) {
        return false;
    }
    const CVector* pos = PositionOf(playerPed);
    float xy[2] = {pos->x, pos->y};
    return Zone_IsPointInside(zone, xy) != 0;
}

// 0x596B00 (thiscall on TheSounds+0x34, a byte of flags: 1 the school's bell rung, 2 its
// bank loaded, 4 the town's rung, 8 its bank, 0x10 in the school's area, 0x20 in the
// town's, 0x40 the heart's bank, 0x80 the heartbeat on): the bells on the hour (the
// school's at 9, 15 and 22) and the heartbeat while the player's +0x1E91 is set
void __fastcall Sounds_UpdateClockTowersImpl(uint8_t* flags) {
    if (currArea == 0) {
        if ((frameCounter & 0xF) == 0) {
            if (PlayerInZone("AMB_SCHOOL_AREA")) {
                *flags |= 0x10;
            } else {
                *flags &= 0xEF;
            }
            if (PlayerInZone("AMB_BUSINESS_AREA")) {
                *flags |= 0x20;
            } else {
                *flags &= 0xDF;
            }
        }
        // the school's
        if (*flags & 0x10) {
            int bank = SoundBanks_Find(soundBanks, "EMITTERS\\MainMap\\sclcktwr.bnk");
            if (IsTimeInRange(8, 9, 0x3A, 0x10) != 0 || IsTimeInRange(0xE, 0xF, 0x3A, 7) != 0 ||
                IsTimeInRange(0x15, 0x16, 0x3A, 0x13) != 0) {
                if ((*flags & 2) == 0) {
                    if (IsPlayerInFastVehicle() == 0) {
                        *flags |= 2;
                        SoundBanks_Load(soundBanks, bank, 1, 1);
                    }
                } else if (SoundBanks_IsLoaded(soundBanks, bank) != 0 && (*flags & 1) == 0) {
                    CVector pos{193.0f, -74.0f, 25.0f};
                    const char* name = nullptr;
                    if (clockHours == 9) {
                        name = "SchClockTwr09";
                    } else if (clockHours == 0xF) {
                        name = "SchClockTwr03";
                    } else if (clockHours == 0x16) {
                        name = "SchClockTwr10";
                    }
                    if (name != nullptr) {
                        *flags |= 1;
                        Sounds_PlayAt(theSounds, HashStreamName(name, 1), &pos, 0xA, nullptr, 0, 0x78, 0, 0, nullptr,
                                      nullptr, 4, 0, 1.0f, 0);
                    }
                }
            } else if (*flags & 2) {
                SoundBanks_Unload(soundBanks, bank, 1, 1);
                *flags &= 0xFC;
            }
        } else if (*flags & 2) {
            SoundBanks_Unload(soundBanks, SoundBanks_Find(soundBanks, "EMITTERS\\MainMap\\sclcktwr.bnk"), 1, 1);
            *flags &= 0xFC;
        }
        // the town's
        if (*flags & 0x20) {
            int bank = SoundBanks_Find(soundBanks, "EMITTERS\\MainMap\\clcktwr.bnk");
            if (uint8_t(clockMinutes - 0xF) <= 0x2A) {
                if (*flags & 8) {
                    SoundBanks_Unload(soundBanks, bank, 1, 1);
                    *flags &= 0xF3;
                }
            } else if ((*flags & 8) == 0) {
                if (IsPlayerInFastVehicle() == 0) {
                    *flags |= 8;
                    SoundBanks_Load(soundBanks, bank, 1, 1);
                }
            } else if (SoundBanks_IsLoaded(soundBanks, bank) != 0 && (*flags & 4) == 0 && clockMinutes == 0) {
                int32_t hour = clockHours;
                if (hour > 0xC) {
                    hour -= 0xC;
                }
                if (hour == 0) {
                    hour = 0xC;
                }
                char name[0x14];
                std::strcpy(name, "Clock Tower ");
                if (hour < 10) {
                    std::strcat(name, "0");
                }
                std::sprintf(name + std::strlen(name), "%d", hour);
                CVector pos{645.0f, -90.0f, 25.0f};
                Sounds_PlayAt(theSounds, HashStreamName(name, 1), &pos, 0xA, nullptr, 0, 0x78, 0, 0, nullptr, nullptr,
                              4, 0, 1.0f, 0);
                *flags |= 4;
            }
        } else if (*flags & 8) {
            SoundBanks_Unload(soundBanks, SoundBanks_Find(soundBanks, "EMITTERS\\MainMap\\clcktwr.bnk"), 1, 1);
            *flags &= 0xF3;
        }
    }
    // the heartbeat
    uint8_t* player = playerPed;
    if (player != nullptr && player[0x1E91] != 0 && At<int32_t>(player, 0x13DC) == 0) {
        int bank = SoundBanks_Find(soundBanks, "Objects_2\\heart.bnk");
        if ((*flags & 0x40) == 0) {
            *flags |= 0x40;
            SoundBanks_Load(soundBanks, bank, 1, 1);
            return;
        }
        if (SoundBanks_IsLoaded(soundBanks, bank) != 0 && (*flags & 0x80) == 0) {
            Sounds_PlayAt(theSounds, HashStreamName("Heartbeat", 1), nullptr, 1, nullptr, 0, 0x78, 0, 0, nullptr,
                          nullptr, 6, 1, 1.0f, 0);
            *flags |= 0x80;
        }
        return;
    }
    if (*flags & 0x40) {
        SoundBanks_Unload(soundBanks, SoundBanks_Find(soundBanks, "Objects_2\\heart.bnk"), 1, 1);
        *flags &= 0x3F;
    }
}

// ---- the loop slots' players --------------------------------------------------------------

// 0x597010: a looping sound at a position kept going through its slot (started in a free
// slot when it isn't playing; `flag`: its pitch follows its vf 0x74)
void __cdecl Sounds_PlayLoopAtImpl(re::Ptr32<uint8_t>* ref, int32_t id, const CVector* pos, int a4, int volume, int pitch,
                                   int a7, int flag) {
    if (*ref != nullptr) {
        if (vc::Call<int32_t>(*ref, 0x58) != id) {
            vc::Call<void>(*ref, 0x34);
            Sound_Set(ref, nullptr);
        }
    }
    uint8_t* sound = *ref;
    if (sound == nullptr) {
        int slot = FreeLoopSlot();
        if (slot == -1) {
            return;
        }
        SoundParams params{};
        params.pos = {0.0f, 0.0f, 0.0f};
        params.flags = 0;
        params.unknown14 = 0;
        params.volume = volume;
        params.at = Sounds_Fn59B740(theSounds, pos, 0);
        params.flags |= 0x17;
        params.pos = *pos;
        Sound_Set(ref, Sfx_Play2(sfxManager, id, &params, 0, 0, 0, 0, 0, 0, 0, reinterpret_cast<void*>(0x0049A650),
                                 0, a4, 6));
        if (*ref != nullptr) {
            vc::Call<void>(*ref, 0x98, a7);
            if (uint8_t(flag) != 0) {
                uint8_t* s = *ref;
                vc::Call<void>(s, 0x78, vc::Call<float>(s, 0xB0));
            }
        }
        Sound_Set(&loopSlots[slot].sound, *ref);
        loopSlots[slot].keep = 1;
        return;
    }
    int slot = LoopSlotOf(sound);
    if (slot == -1) {
        vc::Call<void>(sound, 0x34);
        Sound_Set(ref, nullptr);
        return;
    }
    vc::Call<void>(sound, 0x8C, pos);
    if (volume > 0x400) {
        volume = 0x400;
    }
    float p = Audio_Pitch5A7EA0(pitch);
    if (uint8_t(flag) != 0) {
        float current = vc::Call<float>(*ref, 0xB0);
        float target = vc::Call<float>(*ref, 0x74);
        float delta = target - current;
        vc::Call<void>(*ref, 0x78, current);
        p = Audio_Fn5A7F00(delta + delta, soundTimeStep) + p;
    }
    SetVoices(*ref, volume, p, nullptr);
    loopSlots[slot].keep = 1;
}

// 0x597310: the same with the volume from the sound's own (0x59D440, 0x59E4F0)
void __cdecl Sounds_PlayLoopAtScaled(re::Ptr32<uint8_t>* ref, int32_t id, const CVector* pos, int a4, int pitch, int a6,
                                     int flag) {
    uint8_t* sound = *ref;
    if (sound == nullptr) {
        Sounds_PlayLoopAt(ref, id, pos, a4, 1, pitch, a6, flag);
        return;
    }
    if (LoopSlotOf(sound) == -1) {
        vc::Call<void>(sound, 0x34);
        Sound_Set(ref, nullptr);
        return;
    }
    vc::Call<void>(sound, 0x8C, pos);
    uint8_t* s = *ref;
    int32_t level = SfxItem_Fn59D440(s, vc::Call<float>(s, 0xB0));
    float volume = Audio_Fn59E4F0(float(level), 4);
    Sounds_PlayLoopAt(ref, id, pos, a4, int32_t(volume), pitch, a6, flag);
}

// 0x597400: a looping sound without a position (`arg` to each voice)
void __cdecl Sounds_PlayLoop(re::Ptr32<uint8_t>* ref, int32_t id, int arg, int a4, int volume, int pitch) {
    if (*ref != nullptr) {
        if (vc::Call<int32_t>(*ref, 0x58) != id) {
            vc::Call<void>(*ref, 0x34);
            Sound_Set(ref, nullptr);
        }
    }
    uint8_t* sound = *ref;
    if (sound == nullptr) {
        int slot = FreeLoopSlot();
        if (slot == -1) {
            return;
        }
        SoundParams params{};
        params.pos = {0.0f, 0.0f, 0.0f};
        params.volume = volume;
        params.at = arg;
        params.unknown14 = 0;
        params.flags = 0x15;
        Sound_Set(ref, Sfx_Play2(sfxManager, id, &params, 0, 0, 0, 0, 0, 0, 0, reinterpret_cast<void*>(0x0049A650),
                                 0, a4, 6));
        if (*ref != nullptr) {
            vc::Call<void>(*ref, 0x94, 1);
        }
        Sound_Set(&loopSlots[slot].sound, *ref);
        loopSlots[slot].keep = 1;
        return;
    }
    int slot = LoopSlotOf(sound);
    if (slot == -1) {
        vc::Call<void>(sound, 0x34);
        Sound_Set(ref, nullptr);
        return;
    }
    if (volume > 0x400) {
        volume = 0x400;
    }
    float p = Audio_Pitch5A7EA0(pitch);
    SetVoices(*ref, volume, p, &arg);
    loopSlots[slot].keep = 1;
}

// ---- the update ------------------------------------------------------------------------------

// 0x597610 (thiscall): the audio's per-frame update
void __fastcall Sounds_Update(uint8_t* self) {
    uint32_t t = Timer_GetCurrentTime();
    uint32_t seconds = t / 1000;
    uint32_t ms = t - seconds * 1000;
    int32_t now = int32_t((ms * 1000) / 1000 + seconds * 1000);
    At<int32_t>(self, 0x2B3C) = now;
    if (self[0x2B48] != 0) {
        self[0x2B48] = 0;
        At<int32_t>(self, 0x2B40) += now - At<int32_t>(self, 0x2B44);
    }
    uint8_t paused;
    if (userStart == 0 && flag20C32EA == 0 && self[0x37] == 0 && self[0x38] == 0 && IsInTitleMenu() == 0 &&
        pausedC1A999 == 0 && pausedC1A998 == 0) {
        paused = 0;
    } else {
        paused = 1;
    }
    if (paused != self[0x35]) {
        if (paused) {
            Voices_Pause();
            StreamChannels_Pause(self + 0x4E4);
            VoiceEngine_Pause(voiceEngine, 1);
            self[0x35] = 1;
        } else {
            Voices_Resume();
            StreamChannels_Resume(self + 0x4E4);
            VoiceEngine_Pause(voiceEngine, 0);
            self[0x35] = 0;
        }
        self[0x35] = paused;
    }
    At<float>(self, 0x2350) = timeStepScale * ms_timeStep;
    Audio_Fn5A8850();
    if (*static_cast<re::Ptr32<void>*>(refCF3D88) != nullptr) {
        if (refCF3D88Frames == 0) {
            SetRef(refCF3D88, nullptr);
        } else {
            refCF3D88Frames--;
        }
    }
    SoundBanks_Update(soundBanks);
    Environment_Update(At<void*>(self, 0x4C4));
    FireAlarm_Update(self + 8);
    Sounds_UpdateClockTowers(self + 0x34);
    if (self[0x4D0] == 0) {
        Ambience_Update(ambience);
    }
    StreamChannels_Update(streamChannels);
    SfxManager_Update(sfxManager);
    if (self[0x35] == 0) {
        uint8_t vehicles;
        if (At<uint8_t>(At<uint8_t*>(self, 0x5D4), 0) == 0) {
            vehicles = 1;
        } else {
            // (the arcade games' own vehicle sounds)
            vehicles = MissionRunning("ArcadeRace") != 0 || MissionRunning("ArcadeRace3D") != 0;
        }
        if (vehicles) {
            Sounds_UpdateVehicles(self);
        }
        BikeAudios_Update(At<void*>(self, 0x2B30));
        VehicleAudios_Update(At<void*>(self, 0x2B34));
        MusicStream_Update(self + 0x534);
        AmbienceEmitters_Update(At<void*>(self, 0x4D8));
        StreamEmitter_Update(streamEmitter);
    }
    MusicStream_Update(self + 0x584);
    LoopSounds_Update(At<void*>(self, 0x4D4));
    Sounds_UpdateRats(self);
    RatSounds_Update(At<void*>(self, 0x4DC));
    SpeechLib_Update(self + 0x1070);
    Speech_Update5A5B30(self + 0x5D8);
    FightMusic_Update(self + 0x440);
    Obj58D870_Update(At<void*>(self, 0x4E0));
    InteractiveMusic_Update();
    VoiceEngine_Update(voiceEngine);
    Voices_UpdateTime(m_snTimeInMilliseconds);
    Sounds_UpdateLoopSlots(self);
}

// 0x5978F0 (thiscall): the skateboard's rolling, turning and skidding sounds
void __fastcall SkateAudio_Update(SkateAudio* self) {
    int bank = SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkA.bnk");
    if (SoundBanks_IsLoaded(soundBanks, bank) != 0) {
        SoundBanks_Fn59A7D0(soundBanks, bank);
    }
    bank = SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkB.bnk");
    if (SoundBanks_IsLoaded(soundBanks, bank) != 0) {
        SoundBanks_Fn59A7D0(soundBanks, bank);
    }
    bank = SoundBanks_Find(soundBanks, "vehicles\\skatebrd.bnk");
    self->unknown4 = 3;
    SoundBanks_Request(soundBanks, bank, 5000, 0);
    if (SoundBanks_IsLoaded(soundBanks, bank) == 0) {
        return;
    }
    uint8_t* ped = self->ped;
    if (At<int32_t>(ped, 0x1324) != 0 || At<int32_t>(ped, 0x1470) == 0) {
        return;
    }
    int32_t roll = 0;
    int32_t turn = 0;
    uint8_t allowed = 1;
    if (At<int32_t>(ped, 0x1310) == 0xD && Pad_GetValue(6, 0) < 0.01f) {
        allowed = 0;   // (the player without the pad's input)
    }
    ped = self->ped;
    if (At<int32_t>(ped, 0x1324) != 0) {
        allowed = 0;
    }
    // (the x87 in single precision: the constants are floats)
    float lean = At<float>(ped, 0x138) / 0.045f;
    float vx = At<float>(ped, 0x124);
    float vy = At<float>(ped, 0x128);
    float speed = std::fabs(std::sqrt(std::fabs(vx * vx + vy * vy))) / 0.18f;
    if (lean < -1.0f) {
        lean = -1.0f;
    } else if (lean > 1.0f) {
        lean = 1.0f;
    }
    if (1.0f < speed) {
        speed = 1.0f;
    }
    Audio_ApproachF(&self->lean, lean, 0.1f);
    Audio_ApproachF(&self->speed, speed, 0.1f);
    float curLean = self->lean;
    float curSpeed = self->speed;
    SkateAudio_SkateSounds(self, &roll, &turn, curLean);
    float r = Audio_Fn59E4F0(1024.0f, 4);
    float absLean = std::fabs(curLean);
    int32_t turnVolume = int32_t(absLean * r * curSpeed);
    int32_t turnPitch = int32_t(absLean * 250.0f * curSpeed);
    int32_t rollVolume = int32_t(r * curSpeed * (1.0f - absLean * 0.35f));
    int32_t rollPitch = int32_t(absLean * 150.0f * curSpeed + curSpeed * 100.0f);
    if (roll != 0) {
        Sounds_PlayLoopAt(&self->roll, roll, PositionOf(self->ped), 2, rollVolume, rollPitch, 0, 0);
    }
    ped = self->ped;
    if (allowed && (At<int32_t>(ped, 0x14E4) != 0 || 0.01f < At<float>(ped, 0x1D5C))) {
        CVector velocity{At<float>(ped, 0x124), At<float>(ped, 0x128), 0.0f};
        float skid = Vector_Magnitude2D(&velocity) / 0.2f;
        if (skid >= 1.0f) {
            skid = 1.0f;
        }
        if (0.01f < At<float>(ped, 0x1D5C)) {
            CVector forward;
            Placeable_GetForward(ped, &forward);
            Vector_Normalise(&velocity);
            Vector_Normalise(&forward);
            float dot = std::fabs(DotProduct(&forward, &velocity));
            float twice = skid + skid;
            skid = (1.0f - dot) * twice;
            if (skid >= 1.0f) {
                skid = 1.0f;
            }
        }
        int32_t volume = int32_t(skid * r);
        int32_t pitch = int32_t(skid * 1000.0f * curSpeed);
        int32_t id = HashStreamName("SkateSkidStop", 1);
        Sounds_PlayLoopAt(&self->skid, id, PositionOf(self->ped), 2, volume, pitch, 0, 0);
    } else if (turn != 0) {
        Sounds_PlayLoopAt(&self->skid, turn, PositionOf(self->ped), 2, turnVolume, turnPitch, 0, 0);
    }
    self->lean = curLean;
    self->speed = curSpeed;
}

// 0x597D50: the skater's sounds (the one slot at 0xCF3D88: taken while free; its +4
// counts down the frames it lives on without this)
void __cdecl SkateAudio_Play(uint8_t* ped) {
    SkateAudio* slots = reinterpret_cast<SkateAudio*>(0x00CF3D88);
    SkateAudio* slot = nullptr;
    for (int i = 0; i < 1; i++) {
        if (slots[i].ped == ped) {
            slot = &slots[i];
            break;
        }
        if (slots[i].ped == nullptr) {
            slot = &slots[i];
        }
    }
    if (slot->ped == nullptr) {   // (no slot: null, read anyway)
        SetRef(slot, ped);
        slot->unknown4 = 3;
        Sound_Set(&slot->roll, nullptr);
        Sound_Set(&slot->skid, nullptr);
    }
    SkateAudio_UpdateFn(slot);
}

// ---- registration -----------------------------------------------------------------------------

#define SOUNDMGR2_REVERSED(name, address, fn)                                                            \
    static void* RE_CONCAT(soundmgr2_original_, __LINE__);                                                \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(soundmgr2_original_, __LINE__), \
             re::Mode::Original)

SOUNDMGR2_REVERSED("TheSounds::UpdateVehicles", 0x00596530, &Sounds_UpdateVehiclesImpl);
SOUNDMGR2_REVERSED("TheSounds::UpdateRats", 0x005969D0, &Sounds_UpdateRatsImpl);
SOUNDMGR2_REVERSED("TheSounds::UpdateClockTowers", 0x00596B00, &Sounds_UpdateClockTowersImpl);
SOUNDMGR2_REVERSED("TheSounds::PlayLoopAt", 0x00597010, &Sounds_PlayLoopAtImpl);
SOUNDMGR2_REVERSED("TheSounds::PlayLoopAtScaled", 0x00597310, &Sounds_PlayLoopAtScaled);
SOUNDMGR2_REVERSED("TheSounds::PlayLoop", 0x00597400, &Sounds_PlayLoop);
SOUNDMGR2_REVERSED("TheSounds::Update", 0x00597610, &Sounds_Update);
SOUNDMGR2_REVERSED("SkateAudio::Update", 0x005978F0, &SkateAudio_Update);
SOUNDMGR2_REVERSED("SkateAudio::Play", 0x00597D50, &SkateAudio_Play);

} // namespace
