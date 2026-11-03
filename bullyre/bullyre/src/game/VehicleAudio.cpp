// The vehicles' engine sounds, first part (0x5884B0..0x58A9C0): VehicleAudio's set-up,
// the normal, arcade and small engines, collision sounds and the tyre-turn callback.
// Volumes run 0..1024; the pitches are offsets. See VehicleAudio.h.

#include <cmath>
#include <cstring>

#include "game/VehicleAudio.h"
#include "reversed.h"

namespace {

using namespace vaudio;

GAME_VAR(void*, currentMinigame, 0x020C8704);
GAME_VAR(int32_t, beatCounter, 0x00AE603C);
GAME_ARRAY(const char*, radioBeats, 0x00AE602C);   // CrBeats_Dance, _HipHop1, _Rock, _HipHop2
GAME_VAR(const char*, gearScrapeSound, 0x00AE6018);
GAME_VAR(int32_t, modelA1, 0x00BF430C);
GAME_VAR(int32_t, modelA2, 0x00BF42FC);
GAME_VAR(int32_t, modelA3, 0x00BF431C);
GAME_VAR(int32_t, modelD, 0x00BF4308);
GAME_VAR(int32_t, modelB1, 0x00BF4304);
GAME_VAR(int32_t, modelB2, 0x00BF42F8);
GAME_VAR(int32_t, modelC, 0x00BF4300);
GAME_VAR(int32_t, modelE, 0x00BF4118);

void* Sfx(int id, int volume, void* at) {
    return Sfx_Play(sfxManager, id, volume, at, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 6);
}

int GetId(const char* name) {
    return Sounds_GetId(TheSounds, name);
}

// the forward speed: the move speed along the matrix's forward vector
float ForwardSpeed(void* vehicle) {
    float* m = Entity_GetMatrix(vehicle);
    float* fwd = m + 4;
    float s = At<float>(vehicle, 0x128) * fwd[1] + At<float>(vehicle, 0x124) * fwd[0];
    return s + At<float>(vehicle, 0x12C) * fwd[2];
}

float Handling(void* vehicle, int offset) {
    return At<float>(At<void*>(vehicle, 0x2A4), offset);
}

// ---- small pieces ----------------------------------------------------------------------

// 0x5884B0 (thiscall): a sound pointer made from a sound
re::Ptr32<void>* __fastcall SoundPtr_ConstructImpl(re::Ptr32<void>* self, re::Edx, void* sound) {
    *self = nullptr;
    Sound_Set(self, sound);
    return self;
}

// 0x5884D0 (thiscall): a sound pointer assigned
re::Ptr32<void>* __fastcall SoundPtr_AssignImpl(re::Ptr32<void>* self, re::Edx, const re::Ptr32<void>* other) {
    Sound_Set(self, *other);
    return self;
}

// 0x5884F0 (thiscall): the minigame's value at +0x15450 (0 unless +0x1544C is set)
float __fastcall Minigame_Value(uint8_t* self, re::Edx) {
    if (At<int32_t>(self, 0x1544C) != 0) {
        return At<float>(self, 0x15450);
    }
    return 0.0f;
}

// 0x588520 (thiscall): the constructor
VehicleAudio* __fastcall VehicleAudio_ConstructImpl(VehicleAudio* self, re::Edx) {
    self->vehicle = nullptr;
    self->sound4 = nullptr;
    self->sound8 = nullptr;
    self->soundC = nullptr;
    self->sound10 = nullptr;
    self->sound14 = nullptr;
    self->sound18 = nullptr;
    self->sound1C = nullptr;
    SetRef(&self->vehicle, nullptr);
    Sound_Set(&self->sound4, nullptr);
    Sound_Set(&self->sound8, nullptr);
    Sound_Set(&self->sound18, nullptr);
    Sound_Set(&self->soundC, nullptr);
    Sound_Set(&self->sound10, nullptr);
    Sound_Set(&self->sound1C, nullptr);
    self->turn = 0.0f;
    self->lean = 0.0f;
    self->surface[0] = 0;
    self->surface[1] = 0;
    self->surface[2] = 0;
    self->lastF5C = -1.0f;
    self->surface[3] = 0;
    self->maxF60 = -1.0f;
    self->index = -1;
    self->unknown3C = -1;
    return self;
}

// 0x5885E0 (thiscall): the destructor
void __fastcall VehicleAudio_DestructImpl(VehicleAudio* self, re::Edx) {
    Sound_Set(&self->sound1C, nullptr);
    Sound_Set(&self->sound18, nullptr);
    Sound_Set(&self->sound14, nullptr);
    Sound_Set(&self->sound10, nullptr);
    Sound_Set(&self->soundC, nullptr);
    Sound_Set(&self->sound8, nullptr);
    Sound_Set(&self->sound4, nullptr);
    SetRef(&self->vehicle, nullptr);
}

// 0x588640 (thiscall): everything stopped and the slot freed (its bank unloaded, the
// bike banks loaded back)
void __fastcall VehicleAudio_ResetImpl(VehicleAudio* self, re::Edx) {
    StopSound(self->sound4);
    StopSound(self->sound8);
    StopSound(self->sound18);
    StopSound(self->soundC);
    StopSound(self->sound1C);
    StopSound(self->sound10);
    Sound_Set(&self->sound4, nullptr);
    Sound_Set(&self->sound8, nullptr);
    Sound_Set(&self->sound18, nullptr);
    Sound_Set(&self->soundC, nullptr);
    Sound_Set(&self->sound10, nullptr);
    SetRef(&self->vehicle, nullptr);
    self->lastF5C = -1.0f;
    self->maxF60 = -1.0f;
    self->index = -1;
    self->bankLoaded = 0;
    self->unknown70 = 0;
    if (self->bank != -1) {
        SoundBanks_Unload(soundBanks, self->bank, 1, 1);
        self->bank = -1;
    }
    if (self->bikeBanks) {
        if (SoundBanks_Fn598A50(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkA.bnk"))) {
            SoundBanks_Load(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkA.bnk"), 0, 1);
        }
    }
    if (self->bikeBanks) {
        if (SoundBanks_Fn598A50(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkB.bnk"))) {
            SoundBanks_Load(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkB.bnk"), 0, 1);
        }
    }
}

// 0x588820 (thiscall; the reference is destroyed here): a vehicle's sounds set up: its
// own bank (mower, go-kart, scooter, bulldozer), else "Engine_Idle_<model letter><1..4>"
// and "Engine_Crz_..." (PoCycle_ for class 3), and one in six plays radio beats
void __fastcall VehicleAudio_InitImpl(VehicleAudio* self, re::Edx, EntityRef ref, int index) {
    void* vehicle = ref.entity;
    if (vehicle && index > -1) {
        SoundBanks_Request(soundBanks, SoundBanks_Find(soundBanks, "Engine.bnk"), 5000, 0);
        self->index = index;
        SetRef(&self->vehicle, vehicle);
        CVector* pos = EntityPos(self->vehicle);
        int model = At<int16_t>(vehicle, 0x10E);
        self->bank = -1;
        self->unknown6C = -1;
        self->bankLoaded = 0;
        self->startTime = 0;
        VehicleClass(vehicle);
        self->bikeBanks = 0;
        if (VehicleClass(vehicle) == 6) {
            self->bank = SoundBanks_Find(soundBanks, "MINIGAME\\LwnMwr.bnk");
            self->bikeBanks = 1;
        } else if (VehicleClass(vehicle) == 7) {
            self->bank = SoundBanks_Find(soundBanks, "Vehicles\\GoKart01.bnk");
            self->bikeBanks = 1;
        } else if (VehicleClass(vehicle) == 8) {
            self->bank = SoundBanks_Find(soundBanks, "Vehicles\\Scooter.bnk");
        } else if (model == 0xBE) {
            self->bank = SoundBanks_Find(soundBanks, "Vehicles\\Buldozer.bnk");
        }
        if (self->bikeBanks) {
            if (playerPed && At<void*>(playerPed, 0x1554) == vehicle) {
                if (SoundBanks_Fn598A50(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkA.bnk"))) {
                    SoundBanks_Unload(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkA.bnk"), 1, 1);
                }
                if (SoundBanks_Fn598A50(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkB.bnk"))) {
                    SoundBanks_Unload(soundBanks, SoundBanks_Find(soundBanks, "VEHICLES\\BikeBnkB.bnk"), 1, 1);
                }
                self->bikeBanks = 1;
            } else {
                self->bikeBanks = 0;
            }
        }
        self->unknown70 = 0;
        self->unknown6C = -1;
        Sound_Set(&self->sound1C, nullptr);
        if (self->bank != -1) {
            self->bankLoaded = 0;
            Sound_Set(&self->sound4, nullptr);
            Sound_Set(&self->sound8, nullptr);
            Sound_Set(&self->sound10, nullptr);
            SoundBanks_Load(soundBanks, self->bank, 1, 1);
        } else if (VehicleClass(vehicle) != 10) {
            int counter = beatCounter;
            beatCounter = counter + 1;
            if (beatCounter > 4) {
                beatCounter = 1;
            }
            char suffix[4];
            if (model == modelA1 || model == modelA2 || model == modelA3) {
                suffix[0] = 'A';
            } else if (model == modelD) {
                suffix[0] = 'D';
            } else if (model == modelB1 || model == modelB2) {
                suffix[0] = 'B';
            } else if (model == modelC) {
                suffix[0] = 'C';
            } else if (model == modelE) {
                suffix[0] = 'E';
            } else {
                suffix[0] = VehicleClass(vehicle) != 3 ? 'D' : '0';
            }
            suffix[1] = char('0' + counter);
            suffix[2] = 0;
            bool police = VehicleClass(vehicle) == 3;
            char name[32];
            std::strcpy(name, police ? "PoCycle_Idle_" : "Engine_Idle_");
            std::strcat(name, suffix);
            self->idleSoundId = GetId(name);
            police = VehicleClass(vehicle) == 3;
            std::strcpy(name, police ? "PoCycle_Crz_" : "Engine_Crz_");
            std::strcat(name, suffix);
            self->cruiseSoundId = GetId(name);
            if (!Vehicle_HasSirenModel(vehicle) && RandomInt(0, 6) == 1) {
                const char* beats;
                switch (RandomInt(0, 3)) {
                case 1:
                    beats = radioBeats[1];
                    break;
                case 2:
                    beats = radioBeats[2];
                    break;
                case 3:
                    beats = radioBeats[3];
                    break;
                default:
                    beats = radioBeats[0];
                    break;
                }
                int id = GetId(beats);
                void* sound = Sounds_PlayAt(TheSounds, id, pos, 2, pos, 0, 0x7D, 0x7E, 0x7F, nullptr, self->vehicle, 6, 1,
                                            1.0f, 0);
                Sound_Construct(&self->sound18, sound);
            }
        }
    }
    SetRef(&ref.entity, nullptr);
}

// 0x588D00 (thiscall): the arcade (class 10) engine: idle and cruise from the speed, the
// passby zoom of other arcade cars, and the player's button sounds
void __fastcall VehicleAudio_UpdateArcadeImpl(VehicleAudio* self, re::Edx) {
    void* vehicle = self->vehicle;
    float ratio = std::fabs(Handling(vehicle, 0x8C));
    ratio = ratio / Handling(vehicle, 0x84);
    void* playerVehicle = At<void*>(playerPed, 0x1554);
    if (!playerVehicle) {
        return;
    }
    float idleVol = ratio <= 0.7f ? 1.0f - ratio / 0.7f : 0.0f;
    float cruiseVol = ratio <= 0.35 ? ratio / 0.35f : 1.0f;
    float idlePitch = (1.0f - idleVol) * 1200.0f - 600.0f;
    float cruisePitch = 0.5f <= ratio ? ratio * 800.0f : ratio * 2200.0f - 700.0f;
    float gas = At<float>(vehicle, 0x294);
    if (0.0f < gas) {
        cruisePitch = gas * 300.0f - 150.0f + cruisePitch;
    }
    float playerDist = -1.0f;
    if (vehicle == playerVehicle) {
        idleVol = idleVol * 1024.0f;
        cruiseVol = cruiseVol * 1024.0f;
    } else {
        CVector d;
        CVector* r = VectorSub(&d, EntityPos(playerVehicle), EntityPos(vehicle));
        float sum = r->x * r->x + r->y * r->y;
        playerDist = float(std::sqrt(double(std::fabs(sum))));
        float scale = UnsignedToFloat(SFX_Fn59E3E0(playerDist, 1));
        idleVol = scale * idleVol;
        cruiseVol = scale * cruiseVol;
    }
    if ((Handling(self->vehicle, 0x8C) < 0.0f && 0.0f < At<float>(self->vehicle, 0x298)) || transitionDisplayed) {
        idleVol = 0.0f;
        cruiseVol = 0.0f;
    }
    if (self->sound4) {
        int vol = VCall<int>(self->sound4, 0x10);
        int pitch = int(VCall<float>(self->sound4, 0x20));
        if (Audio_Approach(&vol, int(idleVol), 0x40)) {
            VCall<void>(self->sound4, 0x14, vol);
            idleVol = float(vol);
        }
        if (Audio_Approach(&pitch, int(idlePitch), 0x32)) {
            VCall<void>(self->sound4, 0x24, float(pitch));
            idlePitch = float(pitch);
        }
    }
    if (self->sound8) {
        int vol = VCall<int>(self->sound8, 0x10);
        int pitch = int(VCall<float>(self->sound8, 0x20));
        if (Audio_Approach(&vol, int(cruiseVol), 0x40)) {
            VCall<void>(self->sound8, 0x14, vol);
            cruiseVol = float(vol);
        }
        if (Audio_Approach(&pitch, int(cruisePitch), 0x32)) {
            VCall<void>(self->sound8, 0x24, float(pitch));
            cruisePitch = float(pitch);
        }
    }
    idleVol = Audio_Fn59E4F0(idleVol * 0.0009765625f, 4) * 1024.0f;
    cruiseVol = Audio_Fn59E4F0(cruiseVol * 0.0009765625f, 4) * 1024.0f;
    int idlePitchInt = int(idlePitch);
    SoundSlot_Play597400(&self->sound4, GetId("ArcEngine_Idle"), 0, 1, int(idleVol), idlePitchInt, 0);
    int cruisePitchInt = int(cruisePitch);
    SoundSlot_Play597400(&self->sound8, GetId("ArcEngine_Crz"), 0, 1, int(cruiseVol), cruisePitchInt, 0);
    vehicle = self->vehicle;
    if (vehicle != At<void*>(playerPed, 0x1554) && !self->sound10 && playerDist < 8.0f) {
        void* playerVeh = At<void*>(playerPed, 0x1554);
        CVector relSpeed;
        VectorSub(&relSpeed, &At<CVector>(vehicle, 0x124), &At<CVector>(playerVeh, 0x124));
        float speed = Vector_Magnitude2D(&relSpeed);
        float limit = Handling(self->vehicle, 0x84) / 6.0f;
        if (limit < speed) {
            speed = limit;
        }
        float closing = speed / limit;
        if (0.15f < closing) {
            CVector toPlayer;
            VectorSub(&toPlayer, EntityPos(At<void*>(playerPed, 0x1554)), EntityPos(self->vehicle));
            float approach = DotProduct(&toPlayer, &relSpeed);
            float scale = At<float>(self->vehicle, 0x154) * passbyScale;
            CVector* vehPos = EntityPos(self->vehicle);
            CVector* playerPos = EntityPos(At<void*>(playerPed, 0x1554));
            CVector scaled, ahead, after;
            VectorScale(&scaled, &relSpeed, scale);
            VectorAdd(&ahead, vehPos, &scaled);
            toPlayer = *VectorSub(&after, playerPos, &ahead);
            if (0.0f < approach && DotProduct(&toPlayer, &relSpeed) < 0.0f) {
                int vol = int(Audio_Fn59E4F0(1.0f, 4) * 1024.0f);
                float pitch = (closing - 0.15f) * 800.0f - 400.0f;
                void* sound = Sounds_Play(TheSounds, GetId("PassbyeZoom"), vol, 0, int(pitch));
                Sound_Construct(&self->sound10, sound);
            }
        }
    }
    if (self->vehicle == At<void*>(playerPed, 0x1554) && !transitionDisplayed) {
        int vol = int(Audio_Fn59E4F0(1.0f, 4) * 1024.0f);
        float gasNow = At<float>(self->vehicle, 0x294);
        if (0.0f < gasNow) {
            SoundSlot_Play597400(&self->sound10, GetId("AcclButton"), 0, 1, vol, cruisePitchInt, 0);
        } else if (gasNow < 0.0f) {
            SoundSlot_Play597400(&self->sound10, GetId("RevrsButton"), 0, 1, vol, idlePitchInt, 0);
        }
    }
}

// 0x589390 (thiscall): the normal engine: idle and cruise by the speed over 0.2, the
// siren, and a gear scrape when +0x2E0 drops under 0.5 after peaking at 5 or more
void __fastcall VehicleAudio_UpdateEngineImpl(VehicleAudio* self, re::Edx, float delta) {
    if (self->vehicle) {
        VehicleClass(self->vehicle);
    }
    float sirenVol = UnsignedToFloat(SFX_Fn59E3E0(self->distance, 3));
    float engineVol = UnsignedToFloat(SFX_Fn59E3E0(self->distance, 2)) * 1.1f;
    void* vehicle = self->vehicle;
    float speed = At<float>(vehicle, 0x124) * At<float>(vehicle, 0x124) + At<float>(vehicle, 0x128) * At<float>(vehicle, 0x128);
    speed = float(std::sqrt(double(std::fabs(speed))));
    float ratio = speed / 0.2f;
    if (1.0f < ratio) {
        ratio = 1.0f;
    } else if (ratio <= 0.0f) {
        ratio = 0.0f;
    }
    if (vehicle && VehicleClass(vehicle) == 3) {
        ratio = ratio + ratio;
        if (1.0f < ratio) {
            ratio = 1.0f;
        }
    }
    float cruiseVol = ratio * engineVol;
    float idleVol = engineVol * (1.0f - ratio);
    if (self->sound1C) {
        int vol = VCall<int>(self->sound1C, 0x10);
        if (Audio_Approach(&vol, int(sirenVol), 0x20)) {
            VCall<void>(self->sound1C, 0x14, vol);
        }
        sirenVol = float(vol);
    }
    if (self->sound4) {
        int vol = VCall<int>(self->sound4, 0x10);
        if (Audio_Approach(&vol, int(idleVol), 0x32)) {
            VCall<void>(self->sound4, 0x14, vol);
        }
        idleVol = float(vol);
    }
    if (self->sound8) {
        int vol = VCall<int>(self->sound8, 0x10);
        if (Audio_Approach(&vol, int(cruiseVol), 0x32)) {
            VCall<void>(self->sound8, 0x14, vol);
        }
        cruiseVol = float(vol);
    }
    delta = Audio_Fn5A7F00(delta, soundTimeStep);
    if (!(At<uint8_t>(self->vehicle, 0x33A) & 2)) {
        idleVol = 0.0f;
        cruiseVol = 0.0f;
    }
    sirenVol = Audio_Fn59E4F0(sirenVol, 4);
    idleVol = Audio_Fn59E4F0(idleVol, 4);
    cruiseVol = Audio_Fn59E4F0(cruiseVol, 4);
    int pitch = int(delta);
    SoundSlot_Play597010(&self->sound4, self->idleSoundId, EntityPos(self->vehicle), 2, int(idleVol), pitch, 0, 1);
    SoundSlot_Play597010(&self->sound8, self->cruiseSoundId, EntityPos(self->vehicle), 2, int(cruiseVol), pitch, 0, 1);
    if (self->sound4) {
        void* veh = self->vehicle;
        float value = At<float>(veh, 0x2E0);
        if (0.5f < self->lastF5C && value < 0.5f) {
            if (self->maxF60 >= 5.0f) {
                CVector* pos = EntityPos(veh);
                Sounds_PlayAt(TheSounds, GetId(gearScrapeSound), pos, 2, pos, 0, 0, 0, 0, nullptr, nullptr, 6, 1, 1.0f, 0);
            }
            self->maxF60 = -1.0f;
        }
        self->lastF5C = value;
        if (self->maxF60 < value) {
            self->maxF60 = value;
        }
    }
    void* veh = self->vehicle;
    if (Vehicle_HasSirenModel(veh) && At<uint8_t>(veh, 0x36D)) {
        CVector* pos = EntityPos(veh);
        SoundSlot_Play597010(&self->sound1C, GetId("PoliceCarSiren"), pos, 3, int(sirenVol), pitch, 1, 1);
    }
}

// 0x589790 (thiscall): the small engines (their own bank): accelerating, reversing,
// decelerating, starting, and the minigame's munching; "EngineStop" once the seat empties
void __fastcall VehicleAudio_UpdateSmallEngineImpl(VehicleAudio* self, re::Edx) {
    SoundBanks_Request(soundBanks, self->bank, 5000, 0);
    if (!SoundBanks_IsLoaded(soundBanks, self->bank)) {
        return;
    }
    if (!Seats_GetOccupant(static_cast<uint8_t*>(self->vehicle) + 0x1C4, 0)) {
        if (self->sound4 || self->sound8) {
            CVector* pos = EntityPos(self->vehicle);
            void* sound = Sounds_PlayAt(TheSounds, GetId("EngineStop"), pos, 2, pos, 0, 0, 0, 0, nullptr, nullptr, 6, 1, 1.0f, 0);
            Sound_Set(&self->sound18, sound);
            return;
        }
        if (!self->sound18) {
            At<uint8_t>(self->vehicle, 0x33A) &= 0xFD;
        }
        return;
    }
    CVector* pos = EntityPos(self->vehicle);
    CVector cam, d;
    VectorSub(&d, pos, GetCameraPos(&cam));
    float dist = float(std::sqrt(double(std::fabs(d.x * d.x + d.y * d.y + d.z * d.z))));
    int raw = SFX_Fn59E3E0(dist, 2);
    int vol = Scale1024(int(Audio_Fn59E4F0(float(raw), 4)), 1024);
    void* vehicle = self->vehicle;
    int boost = self->sound10 ? 400 : 0;
    re::Ptr32<void>* munch = &self->sound10;
    float gas = At<float>(vehicle, 0x294);
    if (double(gas) > 0.10000000149011612) {
        SoundSlot_Play597010(&self->sound8, GetId("EngineAccel"), EntityPos(vehicle), 2, vol, boost, 0, 0);
    } else if (double(gas) < -0.10000000149011612) {
        SoundSlot_Play597010(&self->soundC, GetId("EngineRvsAcel"), EntityPos(vehicle), 2, vol, 0, 0, 0);
    } else {
        CVector* at = EntityPos(vehicle);
        int id;
        int pitch;
        if (self->sound4) {
            id = VCall<int>(self->sound4, 0x58);
            pitch = 0;
        } else if (self->sound8) {
            id = GetId("EngineDecel");
            pitch = boost;
        } else if (self->soundC) {
            id = GetId("EngineRvrDecel");
            pitch = boost;
        } else {
            id = GetId("EngineStart");
            pitch = 0;
        }
        SoundSlot_Play597010(&self->sound4, id, at, 2, vol, pitch, 0, 0);
    }
    void* minigame = currentMinigame;
    if (minigame && At<uint8_t>(minigame, 4) && At<int32_t>(minigame, 0x3C) == 0xB &&
        double(Minigame_Fn5884F0(minigame)) > 0.20000000298023224) {
        void* veh = self->vehicle;
        float moving = std::fabs(At<float>(veh, 0x124) * At<float>(veh, 0x124) + At<float>(veh, 0x128) * At<float>(veh, 0x128));
        if (double(moving) > 4.999999873689376e-05) {
            SoundSlot_Play597010(munch, GetId("MunchLoop"), EntityPos(veh), 2, vol, 0, 0, 0);
        }
    }
}

// 0x589B00 (cdecl; the reference is destroyed here): a vehicle's collision sound by its
// class: the go-kart's crash, damage and kerb sounds, the scooter's, the mower's hit, and
// the arcade car's metal bangs and kerb bumps
void __cdecl VehicleAudio_PlayCollisionImpl(EntityRef ref, float impulse, void* hit, int flag) {
    void* vehicle = ref.entity;
    bool driven = re::Flag(flag);
    if (!vehicle || !sfxManager) {
        SetRef(&ref.entity, nullptr);
        return;
    }
    if (VehicleClass(vehicle) == 7) {
        CVector pos = *EntityPos(vehicle);
        float ratio = std::fabs(ForwardSpeed(vehicle));
        ratio = ratio / Handling(vehicle, 0x84);
        float sp = double(ratio) < 1.0 ? ratio : 1.0f;
        if (sp < 0.0f) {
            sp = 0.0f;
        }
        int pitchInt = 0;
        float scaled = impulse / 100.0f;
        float level = scaled;
        VehicleAudio* audio = VehicleAudios_Find(vehicleAudios, vehicle);
        uint8_t isDriver = Seats_IsDriver(static_cast<uint8_t*>(vehicle) + 0x1C4, playerPed);
        if (!audio) {
            SetRef(&ref.entity, nullptr);
            return;
        }
        int crashId = GetId("CartCrash");
        int damageId = GetId("CartDmge");
        int kerbId = GetId("CurbHit");
        int id;
        void* other = At<void*>(vehicle, 0x1A4);
        float pitchF;
        if (other) {
            if (Vehicle_AsAutomobile(other)) {
                CVector rel;
                VectorSub(&rel, &At<CVector>(vehicle, 0x124), &At<CVector>(Vehicle_AsAutomobile(other), 0x124));
                float v = std::fabs(Vector_Magnitude2D(&rel));
                v = v / Handling(vehicle, 0x84);
                v = MaxF(0.0f, MinF(1.0f, v));
                if (!isDriver && double(v) < 0.10000000149011612) {
                    level = 0.0f;
                    goto notDriver;
                }
                level = float(double(v) * 1.7999999523162842);
                level = float(0.10000000149011612 + double(level));
                level = MinF(1.0f, level);
            } else {
                CVector dir, speed;
                VectorSub(&dir, EntityPos(other), EntityPos(vehicle));
                speed = At<CVector>(vehicle, 0x124);
                Vector_Normalise(&dir);
                Vector_Normalise(&speed);
                level = std::fabs(DotProduct(&speed, &dir));
                level = level * sp;
                if (level < 0.0f) {
                    level = 0.0f;
                }
            }
        } else if (!driven) {
            level = impulse / 200.0f;
            if (1.0f < level) {
                level = 1.0f;
            }
        }
        if (!isDriver) {
            goto notDriver;
        }
        if (driven) {
            id = kerbId;
            if (sp < 0.5f) {
                level = sp + sp + 0.2f;
            } else {
                level = (1.0f - sp) * 0.4f + 0.5f;
            }
            pitchF = sp * 800.0f;
            goto play;
        }
        if (50.0f < impulse) {
            impulse = scaled;
            if (1.0f < impulse) {
                impulse = 1.0f;
            }
            level = (impulse * 3.0f + level) * 0.25f;
        }
        if (level < sp) {
            level = (sp + level) * 0.5f;
        }
        if (hit) {
            if (Vehicle_AsAutomobile(hit)) {
                id = crashId;
                pitchF = sp * 500.0f;
                goto play;
            }
            uint8_t type = At<uint8_t>(hit, 0x108) & 7;
            if (type != 2 && (type != 4 || !(At<uint8_t>(hit, 0x1BC) & 4))) {
                SetRef(&ref.entity, nullptr);
                return;
            }
            id = damageId;
            pitchF = sp * 500.0f;
            goto play;
        }
        id = double(level) > 0.800000011920929 ? crashId : damageId;
        goto playNoPitch;
    notDriver:
        if (driven) {
            SetRef(&ref.entity, nullptr);
            return;
        }
        id = double(level) > 0.800000011920929 ? crashId : damageId;
        pitchF = sp * 500.0f;
    play:
        pitchInt = int(pitchF);
    playNoPitch:
        if (audio->sound1C && !driven) {
            SetRef(&ref.entity, nullptr);
            return;
        }
        void* sound = Sounds_PlayAt(TheSounds, id, &pos, 3, nullptr, 0, 0, 0, 0, nullptr, nullptr, 4, 0, level, pitchInt);
        re::Ptr32<void> temp;
        SoundPtr_Construct(&temp, sound);
        if (!driven && isDriver) {
            SoundPtr_Assign(&audio->sound1C, &temp);
        }
        Sound_Set(&temp, nullptr);
        SetRef(&ref.entity, nullptr);
        return;
    }
    if (VehicleClass(vehicle) == 8) {
        CVector pos = *EntityPos(vehicle);
        float sp = std::fabs(ForwardSpeed(vehicle)) / 0.2f;
        bool hard = 0.5f < impulse;
        if (!(1.0f > sp)) {
            sp = 1.0f;
        }
        int crashId = GetId(At<uint8_t>(vehicle, 0x1BF) == 1 ? "Dive_Splash" : "ScooterCrash");
        int hitId = GetId(hit && (At<uint8_t>(hit, 0x108) & 7) == 3 ? "ScooterPedHit" : "ScooterHit");
        void* at;
        float level;
        if (Seats_IsDriver(static_cast<uint8_t*>(vehicle) + 0x1C4, playerPed)) {
            level = Audio_Fn59E4F0(sp * 1024.0f, 4);
            if (!hit) {
                at = Sounds_Fn59B740(TheSounds, &pos, 0);
                Sfx(hard ? crashId : hitId, int(level), at);
                SetRef(&ref.entity, nullptr);
                return;
            }
            if (driven) {
                SetRef(&ref.entity, nullptr);
                return;
            }
            if (hard && (At<uint8_t>(hit, 0x108) & 7) != 3) {
                Sfx(crashId, int(level), reinterpret_cast<void*>(-1));
            } else {
                Sfx(hitId, int(level), reinterpret_cast<void*>(-1));
            }
            SetRef(&ref.entity, nullptr);
            return;
        }
        CVector cam, d;
        GetCameraPos(&cam);
        VectorSub(&d, EntityPos(vehicle), &cam);
        float dist = Vector_Magnitude(&d);
        float volume = UnsignedToFloat(SFX_Fn59E3E0(dist, 4));
        float factor = double(Handling(vehicle, 0x8C)) > 0.001 ? sp * 1024.0f : 700.0f;
        level = volume * factor * 0.0009765625f;
        if (1024.0f < level) {
            level = 1024.0f;
        }
        at = Sounds_Fn59B740(TheSounds, &pos, 0);
        if (driven) {
            SetRef(&ref.entity, nullptr);
            return;
        }
        Sfx(hard ? crashId : hitId, int(level), at);
        SetRef(&ref.entity, nullptr);
        return;
    }
    if (VehicleClass(vehicle) == 6) {
        if (driven && At<uint8_t>(vehicle, 0x1BE) == 8) {
            SetRef(&ref.entity, nullptr);
            return;
        }
        VehicleAudio* audio = VehicleAudios_Find(vehicleAudios, vehicle);
        if (!audio || audio->sound18) {
            SetRef(&ref.entity, nullptr);
            return;
        }
        Entity_GetMatrix(vehicle);
        float level = std::fabs(Handling(vehicle, 0x8C));
        level = level * 1000.0f;
        level = level * 15.5f;
        if (1024.0f < level) {
            level = 1024.0f;
        }
        level = Audio_Fn59E4F0(level, 4);
        Sound_Construct(&audio->sound18, Sfx(GetId("MowerHit"), int(level), reinterpret_cast<void*>(-1)));
        SetRef(&ref.entity, nullptr);
        return;
    }
    if (VehicleClass(vehicle) == 10 && playerPed && !transitionDisplayed && At<void*>(playerPed, 0x1554) == vehicle &&
        At<void*>(vehicle, 0x1A4)) {
        void* other = At<void*>(vehicle, 0x1A4);
        CVector rel;
        VectorSub(&rel, &At<CVector>(vehicle, 0x124), &At<CVector>(other, 0x124));
        float speed = Vector_Magnitude2D(&rel);
        float limit = Handling(vehicle, 0x84) / 6.0f;
        if (limit < speed) {
            speed = limit;
        }
        if ((At<uint8_t>(At<void*>(vehicle, 0x1A4), 0x108) & 7) == 2) {
            float level = speed / limit;
            Sfx(GetId("MetalBangs"), int(level * 1024.0f), reinterpret_cast<void*>(-1));
            SetRef(&ref.entity, nullptr);
            return;
        }
        VehicleAudio* audio = VehicleAudios_Find(vehicleAudios, vehicle);
        if (audio) {
            float level = Vector_Magnitude2D(&At<CVector>(vehicle, 0x124)) / (Handling(vehicle, 0x84) * 0.5f);
            if (!(1.0f > level)) {
                level = 1.0f;
            }
            if (audio->nextCurbBump < timeMs) {
                Sfx(GetId("CurbBump"), int(level * 1024.0f), reinterpret_cast<void*>(-1));
                audio->nextCurbBump = timeMs + 500;
            }
        }
    }
    SetRef(&ref.entity, nullptr);
}

// 0x58A680 (cdecl; the reference is destroyed here): the scooter's brake skid
void __cdecl VehicleAudio_PlayBrakeSkid(EntityRef ref) {
    void* vehicle = ref.entity;
    if (vehicle && sfxManager && VehicleClass(vehicle) == 8 && At<uint8_t>(vehicle, 0x1BF) != 1) {
        CVector pos = *EntityPos(vehicle);
        void* at = Sounds_Fn59B740(TheSounds, &pos, 0);
        Sfx(GetId("TireBrakeSkid"), 0x400, at);
    }
    SetRef(&ref.entity, nullptr);
}

GAME_VAR(uint16_t, dozerRange_AE6E4E, 0x00AE6E4E);   // (the bulldozer sound's range)

// 0x58A740: the tyre-turns sound's callback (cdecl): its volume by the speed, its pitch
// by the average slip of the wheels under 1
void __cdecl TyreTurnsCallback(float, float, void* vehicle, void* sound) {
    if (!vehicle || (At<uint8_t>(vehicle, 0x108) & 7) != 2 || At<int32_t>(vehicle, 0x46C) != 0 || !sound) {
        return;
    }
    Entity_GetMatrix(vehicle);
    float level = std::fabs(Handling(vehicle, 0x8C));
    level = level * 2000.0f;
    if (1024.0f < level) {
        level = 1024.0f;
    }
    level = Audio_Fn59E4F0(level, 4);
    int vol = VCall<int>(sound, 0x10);
    if (Audio_Approach(&vol, int(level), 0x20)) {
        VCall<void>(sound, 0x14, vol);
    }
    float sum = 0.0f;
    float count = 0.0f;
    const float* wheels = &At<float>(vehicle, 0x764);
    for (int i = 0; i < 4; i++) {
        if (wheels[i] < 1.0f) {
            count = count + 1.0f;
            sum = wheels[i] + sum;
        }
    }
    int b8 = VCall<int>(sound, 0xB8);
    if (count != 0.0f) {
        sum = sum / count;
    }
    float speed = std::fabs(Handling(vehicle, 0x8C));
    VoiceList voices = {nullptr, 0, 0};
    float pitch = sum * 900.0f;
    pitch = count * pitch;
    pitch = pitch * speed - 450.0f;
    Sound_GetVoices(sound, &voices);
    for (uint32_t i = 0; i < voices.count; i++) {
        if (voices.data[i] == -1) {
            continue;
        }
        if (Voice_Fn5AF530(voices.data[i])) {
            Voice_Fn5AF870(voices.data[i], vol, 0.0f);
        } else {
            Voice_Fn5AF7B0(voices.data[i], vol, b8, 1);
        }
        float value = Audio_Fn5A7EA0(int(pitch));
        Voice_Fn5AF660(voices.data[i], value);
    }
    if (voices.data) {
        Free(voices.data);
    }
}

// 0x58A9D0: the bulldozer's sound's callback (cdecl): past its range (the first value against
// the word at 0xAE6E4E) the sound told to stop and silent, else its volume by that distance
// (0x59E3E0's, unsigned, times 1.1f), for a vehicle (+0x46C 0) times its +0x2E0 / 15, at most
// 1024; its voices' pitch from the second value
void __cdecl DozerCallback(float distance, float pitchValue, void* vehicle, void* sound) {
    uint8_t* dozer = nullptr;
    if (vehicle && (At<uint8_t>(vehicle, 0x108) & 7) == 2 && At<int32_t>(vehicle, 0x46C) == 0) {
        dozer = static_cast<uint8_t*>(vehicle);
    }
    int32_t range = int32_t(distance);
    float level = 0.0f;
    if (range > int32_t(dozerRange_AE6E4E)) {
        At<uint8_t>(sound, 0x5C) = 1;
    } else {
        uint32_t byDistance = uint32_t(SFX_Fn59E3E0(float(range), 2));
        level = float(double(byDistance) * double(1.1f));
    }
    if (dozer) {
        float scale = float(double(At<float>(dozer, 0x2E0)) / 15.0);
        level = scale * level;
    }
    if (1024.0f < level) {
        level = 1024.0f;
    }
    int b8 = VCall<int>(sound, 0xB8);
    level = Audio_Fn59E4F0(level, 4);
    VoiceList voices = {nullptr, 0, 0};
    Sound_GetVoices(sound, &voices);
    for (uint32_t i = 0; i < voices.count; i++) {
        if (voices.data[i] == -1) {
            continue;
        }
        if (Voice_Fn5AF530(voices.data[i])) {
            Voice_Fn5AF870(voices.data[i], int(level), 0.0f);
        } else {
            Voice_Fn5AF7B0(voices.data[i], int(level), b8, 1);
        }
        float pitch = Audio_Fn5A7EA0(int(pitchValue));
        Voice_Fn5AF660(voices.data[i], pitch);
    }
    if (voices.data) {
        Free(voices.data);
    }
}

} // namespace

#define VAUDIO_REVERSED(name, address, fn)                                                     \
    static void* RE_CONCAT(vaudio_original_, __LINE__);                                       \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(vaudio_original_, __LINE__), \
             re::Mode::Original)

VAUDIO_REVERSED("SoundPtr::SoundPtr", 0x005884B0, &SoundPtr_ConstructImpl);
VAUDIO_REVERSED("SoundPtr::operator=", 0x005884D0, &SoundPtr_AssignImpl);
VAUDIO_REVERSED("Minigame::GetValue15450", 0x005884F0, &Minigame_Value);
VAUDIO_REVERSED("VehicleAudio::VehicleAudio", 0x00588520, &VehicleAudio_ConstructImpl);
VAUDIO_REVERSED("VehicleAudio::~VehicleAudio", 0x005885E0, &VehicleAudio_DestructImpl);
VAUDIO_REVERSED("VehicleAudio::Reset", 0x00588640, &VehicleAudio_ResetImpl);
VAUDIO_REVERSED("VehicleAudio::Init", 0x00588820, &VehicleAudio_InitImpl);
VAUDIO_REVERSED("VehicleAudio::UpdateArcade", 0x00588D00, &VehicleAudio_UpdateArcadeImpl);
VAUDIO_REVERSED("VehicleAudio::UpdateEngine", 0x00589390, &VehicleAudio_UpdateEngineImpl);
VAUDIO_REVERSED("VehicleAudio::UpdateSmallEngine", 0x00589790, &VehicleAudio_UpdateSmallEngineImpl);
VAUDIO_REVERSED("VehicleAudio::PlayCollision", 0x00589B00, &VehicleAudio_PlayCollisionImpl);
VAUDIO_REVERSED("VehicleAudio::PlayBrakeSkid", 0x0058A680, &VehicleAudio_PlayBrakeSkid);
VAUDIO_REVERSED("VehicleAudio::TyreTurnsCallback", 0x0058A740, &TyreTurnsCallback);
VAUDIO_REVERSED("VehicleAudio::DozerCallback", 0x0058A9D0, &DozerCallback);
