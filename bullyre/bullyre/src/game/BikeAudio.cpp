// The bicycles' sounds (0x58C500..0x58E060): one BikeAudio a ridden bike (0x40 bytes) in
// the manager at 0xCF68D0: pedalling, the surface under the wheels, landings and bumps,
// skids; and the boxing match's crowd (0x58D870..0x58DE60). See VehicleAudio.h for the
// shared sound calls.

#include <cmath>

#include "game/VehicleAudio.h"

#include "game/Crt.h"
#include "reversed.h"
namespace {

using namespace vaudio;

struct BikeAudio {
    re::Ptr32<void> bike;             // +0 (a reference)
    re::Ptr32<void> pedalSound;       // +4
    re::Ptr32<void> surfaceSound;     // +8
    re::Ptr32<void> skidSound;        // +0xC
    int32_t pedalState;     // +0x10 (-1)
    int32_t index;          // +0x14 (-1 free)
    int32_t surface;        // +0x18 (-1)
    uint32_t initTime;      // +0x1C
    float lastDistance;     // +0x20
    uint8_t onWater;        // +0x24
    uint8_t changed25;      // +0x25
    uint8_t pad26[2];
    int32_t landings;       // +0x28
    int32_t takeOffs;       // +0x2C
    int32_t unknown30;      // +0x30
    uint32_t landTime;      // +0x34
    uint32_t takeOffTime;   // +0x38
    uint8_t onGround;       // +0x3C
    uint8_t unknown3D;      // +0x3D
    uint8_t pad3E[2];
};
static_assert(sizeof(BikeAudio) == 0x40);

struct BikeAudioManager {
    re::Ptr32<BikeAudio> entries[7];  // +0
    int32_t freeHint;       // +0x1C
    int32_t count;          // +0x20
    int32_t unknown24;      // +0x24
};

struct BoxingCrowd {
    re::Ptr32<void> hiCrowd;          // +0
    re::Ptr32<void> loCrowd;          // +4
    int32_t excitement;     // +8 (0..1024)
    float level;            // +0xC
    float minimum;          // +0x10
    float rise;             // +0x14
    float noise;            // +0x18
    float decay;            // +0x1C
    uint8_t started;        // +0x20
    uint8_t stopping;       // +0x21
    uint8_t starting;       // +0x22
    uint8_t pad23;
};
static_assert(sizeof(BoxingCrowd) == 0x24);

// the sound parameters Sfx_Play2 takes (partly filled: +4, +0x14 and +0x18 are the
// stack's)
struct SoundParams {
    int32_t volume;     // +0
    int32_t unknown4;   // +4
    CVector pos;        // +8
    int32_t unknown14;  // +0x14
    int32_t unknown18;  // +0x18
    re::Ptr32<void> at;           // +0x1C
    uint32_t flags;     // +0x20
};

GAME_VAR(BikeAudioManager*, bikeAudios, 0x00CF68D0);
GAME_VAR(int32_t, lastFrontSurface, 0x00AE6260);
GAME_VAR(int32_t, lastRearSurface, 0x00AE625C);
GAME_VAR(int32_t, cameraMode, 0x00C3CCF8);
GAME_VAR(int32_t, currArea, 0x00BD1008);
GAME_VAR(int32_t, boxingBank, 0x00CF6004);
GAME_VAR(uint8_t, weatherIndex, 0x00A147E0);
GAME_ARRAY(int32_t, weatherTypes, 0x00A147EC);
GAME_ARRAY(int32_t, distanceVolumes, 0x00AE6188);
GAME_ARRAY(uint8_t, emitterTypes, 0x00AE6CDE);  // 0xB8-byte entries: their range (a word) first
GAME_VAR(const char*, roadSound, 0x00AE6144);
GAME_VAR(const char*, gravelSound, 0x00AE6148);
GAME_VAR(const char*, trainTrackSound, 0x00AE614C);
GAME_VAR(const char*, grassSound, 0x00AE6150);
GAME_VAR(const char*, takeOffSound, 0x00AE6128);
inline void* const waterLevels = reinterpret_cast<void*>(0x00BC5DA8);
inline void* const missionMgr = reinterpret_cast<void*>(0x020C3CA0);
constexpr uintptr_t kPedalCallback = 0x0058D090;

GAME_FN(void*, __thiscall, Sfx_Play2, 0x0059E0F0, void* sfx, int id, SoundParams* params, int, int, const CVector* pos, int,
        int, int, int, uintptr_t, uintptr_t, int one, int six);
GAME_FN(uint8_t, __thiscall, WaterLevels_Get, 0x006D4A20, void* levels, float* level, int* flag, const CVector* pos);
GAME_FN(uint8_t, __cdecl, IsAreaInterior, 0x00430320, int area);
GAME_FN(void*, __thiscall, NameKey_Construct, 0x005FA910, uint32_t* self, const char* name);
GAME_FN(void, __thiscall, NameKey_Destruct, 0x0049A650, uint32_t* self);
GAME_FN(int32_t, __thiscall, MissionMgr_FindMission, 0x006A9FD0, void* mgr, uint32_t* key);
GAME_FN(void*, __thiscall, MissionMgr_Fn6AA680, 0x006AA680, void* mgr);
GAME_FN(uint8_t, __thiscall, MissionSlot_Is, 0x006AA410, void* slot, int id);
GAME_FN(void, __thiscall, SoundBanks_Fn59A7D0, 0x0059A7D0, void* banks, int bank);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const CVector* v);
GAME_FN(void*, __cdecl, AsVehicle, 0x00474EE0, void* entity);
inline int __cdecl GameRand() { return crt::Rand(); }

// (BikeAudio's own, where modes apply)
GAME_FN(BikeAudio*, __thiscall, BikeAudio_Construct, 0x0058C500, BikeAudio* self);
GAME_FN(void, __thiscall, BikeAudio_Destruct, 0x0058C570, BikeAudio* self);
GAME_FN(float, __thiscall, Bike_SpeedFactor, 0x0058C5A0, void* bike);
GAME_FN(void, __thiscall, BikeAudio_Reset, 0x0058C650, BikeAudio* self);
GAME_FN(void, __thiscall, BikeAudio_UpdateBumps, 0x0058C700, BikeAudio* self, void* bike);
GAME_FN(uint8_t, __thiscall, BikeAudio_GetSurfaceSound, 0x0058CBC0, BikeAudio* self, void* bike, int* surface, int* sound);
GAME_FN(uint8_t, __thiscall, BikeAudio_GetPedalSound, 0x0058CE10, BikeAudio* self, void* bike, int* state, int* sound);
GAME_FN(void, __thiscall, BikeAudio_Update, 0x0058D250, BikeAudio* self);
GAME_FN(void, __thiscall, BikeAudio_Init, 0x0058D810, BikeAudio* self, void* bike, int index);
GAME_FN(BikeAudio*, __thiscall, BikeAudios_Find, 0x0058DEF0, void* self, void* vehicle);
GAME_FN(int, __thiscall, BikeAudios_FindFree, 0x0058DF60, void* self);

int GetId(const char* name) {
    return Sounds_GetId(TheSounds, name);
}

float Range(int type) {
    return float(*reinterpret_cast<const uint16_t*>(emitterTypes + type * 0xB8));
}

// ---- BikeAudio -------------------------------------------------------------------------

// 0x58C500 (thiscall): the constructor
BikeAudio* __fastcall BikeAudio_ConstructImpl(BikeAudio* self, re::Edx) {
    self->bike = nullptr;
    self->pedalSound = nullptr;
    self->surfaceSound = nullptr;
    self->skidSound = nullptr;
    Sound_Set(&self->pedalSound, nullptr);
    Sound_Set(&self->surfaceSound, nullptr);
    Sound_Set(&self->skidSound, nullptr);
    SetRef(&self->bike, nullptr);
    self->lastDistance = -1.0f;
    self->onGround = 1;
    self->unknown3D = 1;
    self->pedalState = -1;
    self->index = -1;
    self->onWater = 0;
    return self;
}

// 0x58C570 (thiscall): the destructor
void __fastcall BikeAudio_DestructImpl(BikeAudio* self, re::Edx) {
    Sound_Set(&self->skidSound, nullptr);
    Sound_Set(&self->surfaceSound, nullptr);
    Sound_Set(&self->pedalSound, nullptr);
    SetRef(&self->bike, nullptr);
}

// 0x58C5A0 (thiscall, on a CBike): the pedalling's volume factor from the speed (none
// when the wheel barely turns; four times while diving)
float __fastcall Bike_SpeedFactorImpl(uint8_t* bike, re::Edx) {
    float spin = std::fabs(At<float>(bike, 0x19C));
    if (spin < 0.03f) {
        return 0.0f;
    }
    float s = std::fabs(At<float>(bike, 0x694));
    s = s / 0.37f;
    float factor;
    if (s < 0.5f) {
        factor = s * 1.5f;
    } else {
        float t = s - 0.5f;
        factor = (t + t) * 0.25f + 0.75f;
    }
    if (At<uint8_t>(bike, 0x1BF) == 1) {
        factor = factor * 4.0f;
    }
    if (!(1.0f > factor)) {
        factor = 1.0f;
    }
    return factor;
}

// 0x58C650 (thiscall): stopped and freed
void __fastcall BikeAudio_ResetImpl(BikeAudio* self, re::Edx) {
    StopSound(self->pedalSound);
    StopSound(self->surfaceSound);
    StopSound(self->skidSound);
    Sound_Set(&self->skidSound, nullptr);
    Sound_Set(&self->pedalSound, nullptr);
    Sound_Set(&self->surfaceSound, nullptr);
    SetRef(&self->bike, nullptr);
    self->pedalState = -1;
    self->changed25 = 1;
    self->index = -1;
}

// a bump as a wheel rolls onto a rough surface
void PlayBump(void* bike) {
    SoundParams params;
    params.pos = {0.0f, 0.0f, 0.0f};
    float speed = std::fabs(At<float>(bike, 0x694));
    speed = speed * 2.5f * 1024.0f;
    params.volume = int(speed);
    params.flags = 1;
    params.at = Sounds_Fn59B740(TheSounds, EntityPos(bike), 0);
    params.pos = *EntityPos(bike);
    params.flags |= 0x12;
    Sfx_Play2(sfxManager, GetId("BikeBump"), &params, 0, 0, EntityPos(bike), 0, 0, 0, 0, 0, 0, 1, 6);
}

// 0x58C700 (thiscall): the player's bike bumps when a wheel's surface changes to a rough
// one (the last surfaces are globals)
void __fastcall BikeAudio_UpdateBumpsImpl(BikeAudio* self, re::Edx, void* bike) {
    if (!self->bike) {
        return;
    }
    int front = At<uint8_t>(self->bike, 0x5FE);
    if (front != lastFrontSurface) {
        if (front == 9 || front == 6 || front == 1 || front == 7) {
            PlayBump(bike);
        }
        lastFrontSurface = front;
    }
    int rear = At<uint8_t>(self->bike, 0x626);
    if (rear == lastRearSurface) {
        return;
    }
    if (rear == 9 || rear == 6 || rear == 1 || rear == 7) {
        PlayBump(bike);
    }
    lastRearSurface = rear;
}

// 0x58C940 (cdecl): a skid on a surface (the scooter's once; a bike's skid loop)
void __cdecl Bike_PlaySkid(void* bike, int surface, float) {
    if (VehicleClass(bike) == 8) {
        float speed = std::fabs(At<float>(bike, 0x694));
        float level = speed * 1000.0f;
        if (80.0f < level) {
            level = 80.0f;
        }
        if (level < 15.0f) {
            level = 15.0f;
        }
        int volume = int(level);
        const char* name = (surface >= 2 && (surface <= 5 || surface == 0x1B)) ? "MOTSKID_DIRT" : "MOTSKID_CEMENT";
        CVector* pos = EntityPos(bike);
        Sounds_PlayAt(TheSounds, GetId(name), pos, 0, pos, 0, 0, 0, volume, nullptr, nullptr, 6, 1, 1.0f, 0);
        return;
    }
    BikeAudio* audio = nullptr;
    if (bikeAudios) {
        audio = BikeAudios_Find(bikeAudios, bike);
    }
    if (surface == 0x3D || surface == 0x1D || surface == 0x1C || !audio) {
        return;
    }
    float factor = Bike_SpeedFactor(bike) * 2.5f;
    if (factor < 0.3f) {
        factor = factor + factor;
    } else {
        factor = (factor - 0.3f) / 0.7f * 0.4f + 0.6f;
    }
    if (1.0f < factor) {
        factor = 1.0f;
    }
    int volume = int(Audio_Fn59E4F0(1024.0f, 4) * factor);
    if (volume < 50) {
        volume = volume * 12;
    } else if (volume < 600) {
        volume = 600;
    }
    int id = GetId("SKID_CEMENT_NEW");
    if (surface == 2 || surface == 4) {
        id = GetId("SKID_GRAVEL_NEW");
    }
    if (surface == 3) {
        id = GetId("SKID_GRASS_NEW");
    } else if (surface == 0x1B) {
        id = GetId("SKID_SAND_NEW");
    }
    int pitch = int((factor - 0.4f) * 600.0f);
    SoundSlot_Play597010(&audio->skidSound, id, EntityPos(bike), 3, volume, pitch, 0, 0);
}

// 0x58CBC0 (thiscall): the rolling sound for the surface under the front wheel (water
// when in it or diving; snow and ice when it's snowing outdoors)
uint8_t __fastcall BikeAudio_GetSurfaceSoundImpl(BikeAudio* self, re::Edx, void* bike, int* surfaceOut, int* soundOut) {
    int surface = At<uint8_t>(bike, 0x5FE);
    int flag = 1;
    float level;
    uint8_t inWater = WaterLevels_Get(waterLevels, &level, &flag, EntityPos(bike));
    if (inWater || At<uint8_t>(bike, 0x1BF) == 1) {
        surface = 0x1E;
        self->onWater = 1;
    } else {
        self->onWater = inWater;
    }
    if (*surfaceOut != surface) {
        *surfaceOut = surface;
    }
    if (self->onWater) {
        *soundOut = GetId("BIKE_WATER");
        return 1;
    }
    bool snowy = weatherTypes[weatherIndex] == 2 && IsAreaInterior(currArea);
    switch (surface) {
    case 2:
    case 4:
    case 5:
        *soundOut = GetId(gravelSound);
        return 1;
    case 3:
        *soundOut = snowy ? GetId("BIKE_SNOW") : GetId(grassSound);
        return 1;
    case 8:
        *soundOut = GetId("BIKE_CEMSTAIR");
        return 1;
    case 19:
    case 56:
        *soundOut = GetId("BIKE_METAL");
        return 1;
    case 27:
        *soundOut = GetId("BIKE_SAND");
        return 1;
    case 34:
    case 43:
        *soundOut = GetId("BIKE_WOOD");
        return 1;
    case 35:
    case 59:
        *soundOut = GetId(trainTrackSound);
        return 1;
    default:
        *soundOut = snowy ? GetId("BIKE_ICE") : GetId(roadSound);
        return 1;
    }
}

// 0x58CE10 (thiscall): the pedalling sound for the bike's pedal state (braking when the
// player holds the brake at speed); FALSE when unchanged
uint8_t __fastcall BikeAudio_GetPedalSoundImpl(BikeAudio* self, re::Edx, uint8_t* bike, int* stateInOut, int* soundOut) {
    int state = At<int32_t>(bike, 0x714);
    if (At<void*>(self->bike, 0x6BC) == playerPed) {
        if (state != 0 && state != 7 && state != 6 && state != 5) {
            float brake = Pad_GetValue(6, 0);
            if (double(brake) > 0.10000000149011612 && double(At<float>(bike, 0x694)) > 0.10000000149011612 &&
                At<uint8_t>(self->bike, 0x6EC) >= 1) {
                state = 1;
                goto chosen;
            }
        }
    }
    if (state == 0 && std::fabs(At<float>(bike, 0x694)) < 0.02f) {
        state = 1;
    }
chosen:
    *soundOut = 0;
    if (*stateInOut == state) {
        return 0;
    }
    switch (state) {
    case 0:
        if (std::fabs(At<float>(bike, 0x694)) > 0.02f) {
            *soundOut = GetId("CoastSlow");
        }
        break;
    case 1:
        if (std::fabs(At<float>(bike, 0x694)) > 0.02f) {
            if (*stateInOut == -1) {
                self->pedalState = At<int32_t>(bike, 0x714);
                self->changed25 = 1;
                return 0;
            }
            if (!self->onWater) {
                *soundOut = GetId("Brake");
            }
        }
        break;
    case 2: {
        CVector* pos = EntityPos(self->bike);
        Sounds_PlayAt(TheSounds, GetId("Disengage"), pos, 2, pos, 0x3C, 0x3C, 0x7F, 0x7F, nullptr, nullptr, 6, 1, 1.0f, 0);
        *soundOut = GetId("CoastFast");
        break;
    }
    case 3: {
        CVector* pos = EntityPos(self->bike);
        Sounds_PlayAt(TheSounds, GetId("Engage"), pos, 2, pos, 0x3C, 0x3C, 0x7F, 0x7F, nullptr, nullptr, 6, 1, 1.0f, 0);
        *soundOut = GetId("CrankReg");
        break;
    }
    case 4:
        *soundOut = GetId("CrankFast");
        break;
    case 5:
    case 7:
        *soundOut = GetId("CoastFast");
        break;
    case 6:
        *soundOut = GetId("CrankReg");
        break;
    default:
        *soundOut = 0;
        break;
    }
    self->pedalState = state;
    self->changed25 = 1;
    *stateInOut = state;
    return 1;
}

// 0x58D090: the pedalling sound's callback (cdecl), for a bike (+0x46C 1): past 50 (the first
// value, the distance) the sound told to stop; else its volume the distance's (the table at
// 0xAE6188) times the speed factor, at least 200, a quarter without a rider (+0x6EC); the pitch
// the second value's plus 1000 per speed factor over 0.4
void __cdecl PedalCallback(float distance, float pitchValue, void* bike, void* sound) {
    if (!bike || (At<uint8_t>(bike, 0x108) & 7) != 2 || At<int32_t>(bike, 0x46C) != 1) {
        return;
    }
    int32_t d = int32_t(distance);
    if (d > 0x32) {
        At<uint8_t>(sound, 0x5C) = 1;
        return;
    }
    float volume = float(distanceVolumes[d]);
    float factor = Bike_SpeedFactor(bike);
    float base = Audio_Fn5A7EA0(int32_t(pitchValue));
    float pitch = Audio_Fn5A7EA0(int32_t((double(factor) - double(0.4f)) * 1000.0)) + base;
    volume = factor * volume;
    if (volume < 200.0f) {
        volume = 200.0f;
    }
    int b8 = VCall<int>(sound, 0xB8);
    if (At<uint8_t>(bike, 0x6EC) < 1) {
        volume = float(double(volume) * 0.25);
    }
    volume = Audio_Fn59E4F0(volume, 4);
    VoiceList voices = {nullptr, 0, 0};
    Sound_GetVoices(sound, &voices);
    for (uint32_t i = 0; i < voices.count; i++) {
        if (voices.data[i] == -1) {
            continue;
        }
        if (Voice_Fn5AF530(voices.data[i])) {
            Voice_Fn5AF870(voices.data[i], int(volume), 0.0f);
        } else {
            Voice_Fn5AF7B0(voices.data[i], int(volume), b8, 1);
        }
        Voice_Fn5AF660(voices.data[i], pitch);
    }
    if (voices.data) {
        Free(voices.data);
    }
}

// 0x58D250 (thiscall): the bike's update: in range, the rolling loop by distance and
// speed, landing and take-off sounds, and the pedalling (the player's bike bumps too;
// the 3_B mission's bikes louder, with the bike banks requested)
void __fastcall BikeAudio_UpdateImpl(BikeAudio* self, re::Edx) {
    int camera = cameraMode;
    void* bike = self->bike;
    if (!bike) {
        return;
    }
    int type = 2;
    uint8_t mission = 0;
    bool inMission = false;
    if (At<void*>(bike, 0x6BC) != playerPed) {
        uint32_t key;
        int32_t id = MissionMgr_FindMission(missionMgr, static_cast<uint32_t*>(NameKey_Construct(&key, "3_B")));
        inMission = MissionSlot_Is(MissionMgr_Fn6AA680(missionMgr), id) != 0;
        NameKey_Destruct(&key);
    }
    if (inMission) {
        type = 3;
        mission = 1;
    }
    bool player = At<void*>(self->bike, 0x6BC) == playerPed;
    if ((player && currArea != 0x3E) || mission) {
        int skate = SoundBanks_Find(soundBanks, "vehicles\\skatebrd.bnk");
        if (SoundBanks_IsLoaded(soundBanks, skate)) {
            SoundBanks_Fn59A7D0(soundBanks, skate);
        }
        SoundBanks_Request(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkA.bnk"), 6000, 0);
        SoundBanks_Request(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkB.bnk"), 6000, 0);
    }
    CVector cam, d;
    GetCameraPos(&cam);
    VectorSub(&d, EntityPos(self->bike), &cam);
    float dist = float(std::sqrt(double(std::fabs(d.x * d.x + d.y * d.y + d.z * d.z))));
    if (Range(type) < dist) {
        return;
    }
    void* rider = self->bike;
    uint8_t onGround = At<uint8_t>(rider, 0x6EC) >= 1;
    int sound = 0;
    if (self->lastDistance < 0.0f) {
        self->lastDistance = dist;
    }
    float delta = self->lastDistance - dist;
    self->lastDistance = dist;
    float factor = Bike_SpeedFactor(rider);
    float volume = float(distanceVolumes[int(dist)]);
    if (At<void*>(rider, 0x6BC) != playerPed) {
        volume = volume * 2.5f;
    } else {
        volume = volume * 1.1f;
    }
    volume = volume * factor;
    if (mission) {
        volume = volume * 1.25f;
    }
    if (self->onWater) {
        volume = volume * 6.0f;
    }
    if (1024.0f < volume) {
        volume = 1024.0f;
    }
    volume = Audio_Fn59E4F0(volume, 4);
    float step = delta * 150.0f;
    float stepped = Audio_Fn5A7F00(step, soundTimeStep);
    float pitch = Audio_Fn5A7EA0(int((factor - 0.65f) * 1400.0f)) + stepped;
    int surfaceId;
    if (onGround) {
        BikeAudio_GetSurfaceSound(self, self->bike, &self->surface, &sound);
        surfaceId = sound;
    } else {
        self->onWater = 0;
        surfaceId = GetId("CoastAir");
    }
    if (At<void*>(self->bike, 0x6BC)) {
        CVector* pos = EntityPos(self->bike);
        int pitchInt = int(pitch);
        SoundSlot_Play597010(&self->surfaceSound, surfaceId, pos, type, int(volume), pitchInt, 0, 1);
    }
    if (onGround != self->onGround) {
        if (onGround) {
            self->surface = -1;
            if (self->landings != 0 && camera != 3) {
                float threshold = Seats_GetOccupant(static_cast<uint8_t*>(self->bike) + 0x1C4, 0) ? 0.0009f : 0.0036f;
                if (threshold < Vector_MagnitudeSqr(&At<CVector>(self->bike, 0x124)) && self->landTime + 1500 < timeMs) {
                    self->landTime = timeMs;
                }
            }
            self->landings++;
        } else {
            self->takeOffTime = timeMs;
            self->unknown3D = 0;
            if (self->takeOffs != 0 && camera != 3 && At<void*>(self->bike, 0x6BC)) {
                CVector* pos = EntityPos(self->bike);
                Sounds_PlayAt(TheSounds, GetId(takeOffSound), pos, type, pos, 0x3C, 0x3C, 0x7F, 0x7F, nullptr, nullptr, 6, 1,
                              1.0f, 0);
            }
            self->takeOffs++;
        }
    }
    self->onGround = onGround;
    int state = self->pedalState;
    int pedal = 0;
    if (!At<void*>(self->bike, 0x6BC)) {
        if (self->pedalSound) {
            At<uint8_t>(self->pedalSound, 0x5C) = 1;
            Sound_Set(&self->pedalSound, nullptr);
        }
    } else if (BikeAudio_GetPedalSound(self, self->bike, &state, &pedal)) {
        if (self->pedalSound) {
            At<uint8_t>(self->pedalSound, 0x5C) = 1;
        }
        if (!pedal) {
            Sound_Set(&self->pedalSound, nullptr);
        } else if (state == 1 || state == 0) {
            CVector* pos = EntityPos(self->bike);
            void* s = Sounds_PlayAt(TheSounds, pedal, pos, type, pos, 0, 0, 0, 0, nullptr, nullptr, 6, 1, 1.0f, 0);
            Sound_Construct(&self->pedalSound, s);
            self->pedalState = state;
            self->changed25 = 1;
        } else {
            void* vehicle = self->bike;
            void* entity = AsVehicle(vehicle);
            CVector* pos = EntityPos(vehicle);
            void* s = Sounds_PlayAt(TheSounds, pedal, pos, type, pos, 0, 0, 0, 0, reinterpret_cast<void*>(kPedalCallback),
                                    entity, 6, 1, 1.0f, 0);
            Sound_Construct(&self->pedalSound, s);
            self->pedalState = state;
            self->changed25 = 1;
        }
    }
    if (At<void*>(self->bike, 0x6BC) == playerPed) {
        BikeAudio_UpdateBumps(self, self->bike);
    }
}

// 0x58D810 (thiscall): a bike's sounds set up in a slot
void __fastcall BikeAudio_InitImpl(BikeAudio* self, re::Edx, void* bike, int index) {
    if (!bike || index <= -1) {
        return;
    }
    self->index = index;
    SetRef(&self->bike, bike);
    self->pedalState = -1;
    self->surface = -1;
    self->landings = 0;
    self->takeOffs = 0;
    BikeAudio_Update(self);
    self->lastDistance = -1.0f;
    self->onGround = 0;
    self->changed25 = 0;
    self->landTime = 0;
    self->initTime = timeMs;
}

// ---- the boxing crowd ------------------------------------------------------------------

// 0x58D870 (thiscall): the constructor
BoxingCrowd* __fastcall BoxingCrowd_Construct(BoxingCrowd* self, re::Edx) {
    self->hiCrowd = nullptr;
    self->loCrowd = nullptr;
    self->minimum = 200.0f;
    self->level = 200.0f;
    self->decay = 0.95f;
    self->started = 0;
    self->excitement = 200;
    self->noise = 50.0f;
    self->rise = 64.0f;
    Sound_Set(&self->hiCrowd, nullptr);
    Sound_Set(&self->loCrowd, nullptr);
    self->stopping = 0;
    self->starting = 0;
    return self;
}

// 0x58D8E0 (thiscall): the destructor
void __fastcall BoxingCrowd_Destruct(BoxingCrowd* self, re::Edx) {
    Sound_Set(&self->loCrowd, nullptr);
    Sound_Set(&self->hiCrowd, nullptr);
}

void* PlayFrontend(const char* name) {
    return Sounds_PlayAt(TheSounds, GetId(name), nullptr, 0, nullptr, 0, 0x78, 0, 0, nullptr, nullptr, 6, 1, 1.0f, 0);
}

// 0x58D900 (thiscall, this unread): a boo
void __fastcall BoxingCrowd_PlayBoo(BoxingCrowd*, re::Edx) {
    PlayFrontend("Boo");
}

void RaiseLevel(BoxingCrowd* self) {
    float level = self->level + 100.0f;
    self->level = level;
    if (!(1024.0f > level)) {
        self->level = 1024.0f;
    }
}

// 0x58D940 (thiscall): a cheer (rand() / 32767 picks it: nearly always "Oh_1") and the
// crowd louder
void __fastcall BoxingCrowd_PlayCheer(BoxingCrowd* self, re::Edx) {
    switch (GameRand() / 32767) {
    case 0:
        PlayFrontend("Oh_1");
        break;
    case 1:
        PlayFrontend("Oh_2");
        break;
    case 2:
        PlayFrontend("Yeah");
        break;
    }
    RaiseLevel(self);
}

// 0x58DA20 (thiscall): louder
void __fastcall BoxingCrowd_Raise(BoxingCrowd* self, re::Edx) {
    RaiseLevel(self);
}

// 0x58DA50 (thiscall): quieter
void __fastcall BoxingCrowd_Lower(BoxingCrowd* self, re::Edx) {
    self->level = self->level - 100.0f;
}

// 0x58DA60 (thiscall): the excitement (1024 at most)
void __fastcall BoxingCrowd_SetExcitement(BoxingCrowd* self, re::Edx, int excitement) {
    if (excitement > 0x400) {
        excitement = 0x400;
    }
    self->excitement = excitement;
}

// 0x58DA80 (thiscall): the crowd's bank loaded (the loops start once it is)
void __fastcall BoxingCrowd_Start(BoxingCrowd* self, re::Edx) {
    if (boxingBank == -1) {
        return;
    }
    Sound_Set(&self->hiCrowd, nullptr);
    Sound_Set(&self->loCrowd, nullptr);
    SoundBanks_Load(soundBanks, boxingBank, 1, 1);
    self->starting = 1;
}

// 0x58DAC0 (thiscall): the loops cut and the bank unloaded
void __fastcall BoxingCrowd_Kill(BoxingCrowd* self, re::Edx) {
    CutSound(&self->hiCrowd);
    CutSound(&self->loCrowd);
    if (boxingBank == -1) {
        return;
    }
    Sound_Set(&self->hiCrowd, nullptr);
    Sound_Set(&self->loCrowd, nullptr);
    SoundBanks_Unload(soundBanks, boxingBank, 1, 1);
    self->stopping = 1;
}

void SetVoices(void* sound, int volume, float pitch) {
    VoiceList voices = {nullptr, 0, 0};
    Sound_GetVoices(sound, &voices);
    for (uint32_t i = 0; i < voices.count; i++) {
        if (voices.data[i] == -1) {
            continue;
        }
        if (Voice_Fn5AF530(voices.data[i])) {
            Voice_Fn5AF870(voices.data[i], volume, 0.0f);
        } else {
            Voice_Fn5AF7B0(voices.data[i], volume, 0, 1);
        }
        Voice_Fn5AF660(voices.data[i], pitch);
    }
    if (voices.data) {
        Free(voices.data);
    }
}

// 0x58DB40 (thiscall): the crowd's update: the loops started, the level drifting with
// noise and decaying to its minimum, the excitement rising with it, and the two loops
// mixed by the excitement
void __fastcall BoxingCrowd_Update(BoxingCrowd* self, re::Edx) {
    if (!self->started) {
        if (self->starting && SoundBanks_IsLoaded(soundBanks, boxingBank)) {
            self->started = 1;
            self->starting = 0;
        }
        if (!self->started) {
            return;
        }
    }
    if (self->stopping) {
        if (!SoundBanks_IsLoaded(soundBanks, boxingBank)) {
            self->started = 0;
            self->stopping = 0;
        }
        return;
    }
    if (!self->hiCrowd && !self->loCrowd) {
        Sound_Set(&self->loCrowd, Sounds_PlayAt(TheSounds, GetId("Lo_Crowd"), nullptr, 8, nullptr, 0, 0x78, 0, 0, nullptr,
                                                nullptr, 6, 1, 1.0f, 0));
        Sound_Set(&self->hiCrowd, Sounds_PlayAt(TheSounds, GetId("Hi_Crowd"), nullptr, 8, nullptr, 0, 0x78, 0, 0, nullptr,
                                                nullptr, 6, 1, 1.0f, 0));
    }
    float noise = self->noise;
    float random = float(GameRand()) / 32767.0f;
    noise = 0.0f + (noise - 0.0f) * random;
    float level = self->level + noise;
    self->level = level;
    if (!(1024.0f > level)) {
        self->level = 1024.0f;
    }
    level = self->decay * self->level;
    self->level = level;
    if (self->minimum > level) {
        self->level = self->minimum;
    }
    int excitement = self->excitement;
    if (float(excitement) < self->level) {
        self->excitement = int(self->rise) + excitement;
    }
    if (!(1024.0f > self->level)) {
        self->level = 1024.0f;
    }
    if (!self->hiCrowd || !self->loCrowd) {
        return;
    }
    VCall<void>(self->loCrowd, 0x14, 0x400 - self->excitement);
    float pitch = Audio_Fn5A7EA0(self->excitement);
    SetVoices(self->loCrowd, 0x400 - self->excitement, pitch);
    VCall<void>(self->hiCrowd, 0x14, self->excitement);
    SetVoices(self->hiCrowd, self->excitement, pitch);
}

// ---- the bike manager (and the managers' shared pieces) --------------------------------

// 0x58DE60 (thiscall): the constructor (its entries allocated)
BikeAudioManager* __fastcall BikeAudios_Construct(BikeAudioManager* self, re::Edx, int count, int unknown) {
    self->count = count;
    self->unknown24 = unknown;
    for (int i = 0; i < count; i++) {
        void* p = Malloc(0x40);
        self->entries[i] = p ? BikeAudio_Construct(static_cast<BikeAudio*>(p)) : nullptr;
    }
    self->freeHint = 0;
    return self;
}

// 0x58DEC0 (thiscall): every used entry reset
void __fastcall BikeAudios_ResetAll(BikeAudioManager* self, re::Edx) {
    for (int i = 0; i < self->count; i++) {
        BikeAudio* e = self->entries[i];
        if (e && e->index > -1) {
            BikeAudio_Reset(e);
        }
    }
}

// 0x58DEF0 (thiscall): a vehicle's entry (either manager: their layouts match), or null
void* __fastcall Audios_Find(BikeAudioManager* self, re::Edx, void* vehicle) {
    for (int i = 0; i < self->count; i++) {
        if (self->entries[i]->bike == vehicle) {
            return self->entries[i];
        }
    }
    return nullptr;
}

// 0x58DF20 (thiscall): a bike's entry reset (and hinted free)
void __fastcall BikeAudios_Remove(BikeAudioManager* self, re::Edx, void* bike) {
    if (!bike) {
        return;
    }
    for (int i = 0; i < self->count; i++) {
        BikeAudio* e = self->entries[i];
        if (e && e->bike == bike) {
            if (i > -1) {
                BikeAudio_Reset(e);
                self->freeHint = i;
            }
            return;
        }
    }
}

// 0x58DF60 (thiscall): a free entry: the hint, else one with no vehicle and no first two
// sounds (either manager)
int __fastcall Audios_FindFree(BikeAudioManager* self, re::Edx) {
    int hint = self->freeHint;
    if (hint > -1) {
        self->freeHint = -1;
        return hint;
    }
    for (int i = 0; i < self->count; i++) {
        BikeAudio* e = self->entries[i];
        if (e && !e->bike && !e->pedalSound && !e->surfaceSound) {
            return i;
        }
    }
    return -1;
}

// 0x58DFA0 (thiscall): every bike's sounds updated (freed ones reset)
void __fastcall BikeAudios_Update(BikeAudioManager* self, re::Edx) {
    for (int i = 0; i < self->count; i++) {
        BikeAudio* e = self->entries[i];
        if (!e) {
            continue;
        }
        if (e->bike) {
            BikeAudio_Update(e);
        } else if (e->index > -1) {
            BikeAudio_Reset(e);
        }
    }
}

// 0x58DFE0 (thiscall): the entries deleted
void __fastcall BikeAudios_Shutdown(BikeAudioManager* self, re::Edx) {
    for (int i = 0; i < self->count; i++) {
        BikeAudio* e = self->entries[i];
        if (e) {
            BikeAudio_Destruct(e);
            Free(e);
        }
        self->entries[i] = nullptr;
    }
}

// 0x58E020 (thiscall): a ridden bike given a free entry unless it has one
void __fastcall BikeAudios_Add(BikeAudioManager* self, re::Edx, void* bike) {
    if (!At<void*>(bike, 0x6BC)) {
        return;
    }
    for (int i = 0; i < self->count; i++) {
        if (self->entries[i]->bike == bike) {
            return;
        }
    }
    int index = BikeAudios_FindFree(self);
    if (index > -1) {
        BikeAudio_Init(self->entries[index], bike, index);
    }
}

} // namespace

#define BIKEAUDIO_REVERSED(name, address, fn)                                                  \
    static void* RE_CONCAT(bikeaudio_original_, __LINE__);                                    \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(bikeaudio_original_, __LINE__), \
             re::Mode::Original)

BIKEAUDIO_REVERSED("BikeAudio::BikeAudio", 0x0058C500, &BikeAudio_ConstructImpl);
BIKEAUDIO_REVERSED("BikeAudio::~BikeAudio", 0x0058C570, &BikeAudio_DestructImpl);
BIKEAUDIO_REVERSED("BikeAudio::SpeedFactor", 0x0058C5A0, &Bike_SpeedFactorImpl);
BIKEAUDIO_REVERSED("BikeAudio::Reset", 0x0058C650, &BikeAudio_ResetImpl);
BIKEAUDIO_REVERSED("BikeAudio::UpdateBumps", 0x0058C700, &BikeAudio_UpdateBumpsImpl);
BIKEAUDIO_REVERSED("BikeAudio::PlaySkid", 0x0058C940, &Bike_PlaySkid);
BIKEAUDIO_REVERSED("BikeAudio::GetSurfaceSound", 0x0058CBC0, &BikeAudio_GetSurfaceSoundImpl);
BIKEAUDIO_REVERSED("BikeAudio::GetPedalSound", 0x0058CE10, &BikeAudio_GetPedalSoundImpl);
BIKEAUDIO_REVERSED("BikeAudio::PedalCallback", 0x0058D090, &PedalCallback);
BIKEAUDIO_REVERSED("BikeAudio::Update", 0x0058D250, &BikeAudio_UpdateImpl);
BIKEAUDIO_REVERSED("BikeAudio::Init", 0x0058D810, &BikeAudio_InitImpl);
BIKEAUDIO_REVERSED("BoxingCrowd::BoxingCrowd", 0x0058D870, &BoxingCrowd_Construct);
BIKEAUDIO_REVERSED("BoxingCrowd::~BoxingCrowd", 0x0058D8E0, &BoxingCrowd_Destruct);
BIKEAUDIO_REVERSED("BoxingCrowd::PlayBoo", 0x0058D900, &BoxingCrowd_PlayBoo);
BIKEAUDIO_REVERSED("BoxingCrowd::PlayCheer", 0x0058D940, &BoxingCrowd_PlayCheer);
BIKEAUDIO_REVERSED("BoxingCrowd::Raise", 0x0058DA20, &BoxingCrowd_Raise);
BIKEAUDIO_REVERSED("BoxingCrowd::Lower", 0x0058DA50, &BoxingCrowd_Lower);
BIKEAUDIO_REVERSED("BoxingCrowd::SetExcitement", 0x0058DA60, &BoxingCrowd_SetExcitement);
BIKEAUDIO_REVERSED("BoxingCrowd::Start", 0x0058DA80, &BoxingCrowd_Start);
BIKEAUDIO_REVERSED("BoxingCrowd::Kill", 0x0058DAC0, &BoxingCrowd_Kill);
BIKEAUDIO_REVERSED("BoxingCrowd::Update", 0x0058DB40, &BoxingCrowd_Update);
BIKEAUDIO_REVERSED("BikeAudios::BikeAudios", 0x0058DE60, &BikeAudios_Construct);
BIKEAUDIO_REVERSED("BikeAudios::ResetAll", 0x0058DEC0, &BikeAudios_ResetAll);
BIKEAUDIO_REVERSED("VehicleAudios::Find", 0x0058DEF0, &Audios_Find);
BIKEAUDIO_REVERSED("BikeAudios::Remove", 0x0058DF20, &BikeAudios_Remove);
BIKEAUDIO_REVERSED("VehicleAudios::FindFree", 0x0058DF60, &Audios_FindFree); // callers rely on ecx: BikeAudios::Update 0x58DFA0 (turn on together)
BIKEAUDIO_REVERSED("BikeAudios::Update", 0x0058DFA0, &BikeAudios_Update);
BIKEAUDIO_REVERSED("BikeAudios::Shutdown", 0x0058DFE0, &BikeAudios_Shutdown);
BIKEAUDIO_REVERSED("BikeAudios::Add", 0x0058E020, &BikeAudios_Add);
