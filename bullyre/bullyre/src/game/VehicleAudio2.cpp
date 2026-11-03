// The vehicles' engine sounds, second part (0x58AB90..0x58C500): the tyres, the go-kart's
// and the scooter's engines, VehicleAudio's update, and the manager (0xCF68D4). See
// VehicleAudio.h.

#include <cmath>

#include "game/VehicleAudio.h"

namespace {

using namespace vaudio;

int GetId(const char* name) {
    return Sounds_GetId(TheSounds, name);
}

float Handling(void* vehicle, int offset) {
    return At<float>(At<void*>(vehicle, 0x2A4), offset);
}

float ForwardSpeed(void* vehicle) {
    float* m = Entity_GetMatrix(vehicle);
    float* fwd = m + 4;
    float s = At<float>(vehicle, 0x128) * fwd[1] + At<float>(vehicle, 0x124) * fwd[0];
    return s + At<float>(vehicle, 0x12C) * fwd[2];
}

void* Seats(void* vehicle) {
    return static_cast<uint8_t*>(vehicle) + 0x1C4;
}

// VehicleAudio::Init (0x588820) with the reference built in its argument slot, as the
// game does (the callee destroys it there)
// (the 64-bit build: a registered reference built here, passed by value as the game does)
struct EntityRef64 {
    void* entity;
};
__declspec(noinline) void CallInit(VehicleAudio* self, void* vehicle, int index) {
#if defined(_M_IX86)
    __asm {
        push index
        push 0
        mov eax, esp
        push vehicle
        mov ecx, eax
        mov eax, 0x005D7F10
        call eax
        mov ecx, self
        mov eax, 0x00588820
        call eax
    }
#else
    EntityRef64 ref = {nullptr};
    re::Ours<void(__fastcall*)(void*, void*, void*)>(0x005D7F10)(&ref, nullptr, vehicle);
    re::Ours<void(__fastcall*)(VehicleAudio*, void*, EntityRef64, int)>(0x00588820)(self, nullptr, ref, index);
#endif
}

// VehicleAudio::PlayCollision (0x589B00, cdecl) likewise
__declspec(noinline) void CallPlayCollision(void* vehicle, float impulse, void* hit, int flag) {
#if defined(_M_IX86)
    __asm {
        push flag
        push hit
        push impulse
        push 0
        mov ecx, esp
        push vehicle
        mov eax, 0x005D7F10
        call eax
        mov eax, 0x00589B00
        call eax
        add esp, 0x10
    }
#else
    EntityRef64 ref = {nullptr};
    re::Ours<void(__fastcall*)(void*, void*, void*)>(0x005D7F10)(&ref, nullptr, vehicle);
    re::Ours<void(__cdecl*)(EntityRef64, float, void*, int)>(0x00589B00)(ref, impulse, hit, flag);
#endif
}

// ---- VehicleAudio ----------------------------------------------------------------------

// 0x58AB90 (thiscall): the tyres: the go-kart's brake skid, the scooter's tyre turns
// (stopped when not braking, slow or diving)
void __fastcall VehicleAudio_UpdateTyresImpl(VehicleAudio* self, re::Edx) {
    void* vehicle = self->vehicle;
    if (!vehicle || (At<uint8_t>(vehicle, 0x108) & 7) != 2) {
        return;
    }
    if (VehicleClass(vehicle) == 7) {
        void* v = self->vehicle;
        if (v && (At<uint8_t>(v, 0x108) & 7) == 2 && At<int32_t>(v, 0x46C) == 0) {
            float brake = At<float>(vehicle, 0x298);
            float speed = std::fabs(ForwardSpeed(vehicle));
            if (brake == 0.0f || double(speed) <= 0.02 || At<uint8_t>(vehicle, 0x1BF) == 1) {
                CutSound(&self->sound10);
            } else {
                float r = std::fabs(speed);
                r = r / Handling(vehicle, 0x84);
                r = MaxF(0.0f, MinF(1.0f, r));
                int slip = int(300.0f * r);
                int raw = int(r * 424.0f + 600.0f);
                int vol = int(Audio_Fn59E4F0(float(raw), 4));
                SoundSlot_Play597310(&self->sound10, GetId("TireBrakeSkid"), EntityPos(self->vehicle), 3, vol,
                                     slip != 0 ? 1 : 0, 0);
            }
        }
    }
    if (VehicleClass(vehicle) == 8) {
        float brake = At<float>(vehicle, 0x298);
        Entity_GetMatrix(vehicle);
        if (brake == 0.0f || double(std::fabs(ForwardSpeed(vehicle))) <= 0.02 || At<uint8_t>(vehicle, 0x1BF) == 1) {
            CutSound(&self->sound10);
            return;
        }
        if (self->sound10) {
            return;
        }
        CVector* pos = EntityPos(self->vehicle);
        void* sound = Sounds_PlayAt(TheSounds, GetId("TireTurns"), pos, 0, pos, 0, 0x7D, 0x7E, 0x7F,
                                    reinterpret_cast<void*>(kTyreTurnsCallback), self->vehicle, 6, 1, 1.0f, 0);
        Sound_Construct(&self->sound10, sound);
    }
}

// 0x58AE70 (thiscall): the go-kart ("K1_"): starting after a random delay, idle and
// fast engine loops by the speed, burn-outs, tyre turns, and a bump on each wheel's
// change to a rough surface
void __fastcall VehicleAudio_UpdateGoKartImpl(VehicleAudio* self, re::Edx) {
    SoundBanks_Request(soundBanks, self->bank, 5000, 0);
    if (!SoundBanks_IsLoaded(soundBanks, self->bank)) {
        return;
    }
    void* vehicle = self->vehicle;
    float forward = ForwardSpeed(vehicle);
    float ratio = std::fabs(forward) / Handling(vehicle, 0x84);
    float clamped = ratio <= 1.0f ? ratio : 1.0f;
    float sp = double(clamped) < 0.0 ? 0.0f : clamped;
    float turnRate = At<float>(vehicle, 0x138) / 0.028f;
    float turn = turnRate <= 1.0f ? turnRate : 1.0f;
    float gasAbs = std::fabs(At<float>(vehicle, 0x294));
    if (!Seats_GetOccupant(Seats(vehicle), 0)) {
        if (self->sound4) {
            void* sound = Sounds_PlayAt(TheSounds, GetId("K1_EngStop"), EntityPos(self->vehicle), 2, nullptr, 0, 0, 0, 0,
                                        nullptr, nullptr, 6, 1, 1.0f, 0);
            Sound_Set(&self->soundC, sound);
            Sound_Set(&self->sound4, nullptr);
            return;
        }
        if (!self->soundC) {
            At<uint8_t>(self->vehicle, 0x33A) &= 0xFD;
        }
        return;
    }
    if (!self->sound8 && !self->sound4) {
        if (self->startTime == 0 && Seats_IsDriver(Seats(self->vehicle), playerPed)) {
            uint32_t now = timeMs;
            self->startTime = now + RandomInt(0, 500);
            return;
        }
        if (self->startTime > timeMs) {
            return;
        }
        Sounds_PlayAt(TheSounds, GetId("K1_EngStart"), EntityPos(self->vehicle), 2, nullptr, 0, 0, 0, 0, nullptr, nullptr, 6,
                      1, 1.0f, 0);
    }
    int fastId;
    if (Seats_IsDriver(Seats(self->vehicle), playerPed)) {
        fastId = GetId("K1_EngMed");
        self->pitchOffset = 0;
        bool slowForward = forward <= 0.08f && forward >= 0.0f;
        bool play;
        if (slowForward && !self->sound18) {
            play = double(gasAbs) > 0.800000011920929;
        } else {
            play = self->sound18 && double(gasAbs) > 0.2;
        }
        if (play) {
            float level = gasAbs * 1024.0f;
            int vol = int(Audio_Fn59E4F0(level, 4));
            SoundSlot_Play597010(&self->sound18, GetId("TireBurnOut"), EntityPos(self->vehicle), 2, vol, 0, 0, 1);
        }
    } else {
        fastId = GetId("K1_EngFst");
        self->pitchOffset = RandomInt(-175, 175);
    }
    CVector* pos = EntityPos(self->vehicle);
    CVector cam, d;
    VectorSub(&d, pos, GetCameraPos(&cam));
    float dist = float(std::sqrt(double(std::fabs(d.x * d.x + d.y * d.y + d.z * d.z))));
    int idleFactor = 0;
    if (!(double(sp) > 0.800000011920929)) {
        if (0.1f > sp) {
            idleFactor = 0x400;
        } else {
            idleFactor = int((1.0f - (sp - 0.1f) / 0.7f) * 1024.0f);
        }
    }
    int idlePitch = int((1.0f - sp) * 1000.0f - 350.0f);
    int fastFactor;
    if (Seats_IsDriver(Seats(self->vehicle), playerPed)) {
        if (sp < 0.2f) {
            fastFactor = int(sp / 0.2f * 600.0f);
        } else {
            fastFactor = int((sp - 0.2f) / 0.8f * 424.0f + 600.0f);
        }
    } else {
        fastFactor = int(1024.0f * sp);
    }
    int fastPitch = int(sp * 1200.0f - 400.0f + float(self->pitchOffset));
    if (Seats_IsDriver(Seats(self->vehicle), playerPed)) {
        float gas200 = gasAbs * 200.0f;
        float level = float(fastFactor) + gas200;
        if (!(level < 1024.0f)) {
            level = 1024.0f;
        }
        fastFactor = int(level);
        fastPitch += int(gas200 + float(fastPitch));
    }
    if (self->sound8) {
        int vol = VCall<int>(self->sound8, 0x10);
        float pitch = VCall<float>(self->sound8, 0x20);
        if (Audio_Approach(&vol, fastFactor, 0x64)) {
            VCall<void>(self->sound8, 0x14, vol);
        }
        if (Audio_ApproachF(&pitch, float(fastPitch), 50.0f)) {
            VCall<void>(self->sound8, 0x24, pitch);
        }
        fastFactor = vol;
        fastPitch = int(pitch);
    }
    int master = int(Audio_Fn59E4F0(float(SFX_Fn59E3E0(dist, 2)), 4));
    int idleVol = Scale1024(master, idleFactor);
    int fastVol = Scale1024(master, fastFactor);
    SoundSlot_Play597010(&self->sound4, GetId("K1_EngIdle"), EntityPos(self->vehicle), 2, idleVol, idlePitch, 0, 1);
    SoundSlot_Play597010(&self->sound8, fastId, EntityPos(self->vehicle), 2, fastVol, fastPitch, 0, 1);
    if (Seats_IsDriver(Seats(self->vehicle), playerPed)) {
        int raw = int(turn * sp * 1024.0f);
        int vol = int(Audio_Fn59E4F0(float(raw), 4));
        float away = std::fabs(turn - 0.2f);
        int pitch = int((away * 800.0f - 300.0f) * sp);
        SoundSlot_Play597010(&self->sound14, GetId("TireTurns"), EntityPos(self->vehicle), 2, vol, pitch, 0, 1);
    }
    VehicleAudio_UpdateTyres(self);
    for (int i = 0; i < 4; i++) {
        void* v = self->vehicle;
        uint8_t* automobile = (v && (At<uint8_t>(v, 0x108) & 7) == 2) ? static_cast<uint8_t*>(v) : nullptr;
        int surface = automobile[i * 0x28 + 0x6E6];
        if (surface != self->surface[i]) {
            if (surface == 9 || surface == 6 || surface == 1 || surface == 7) {
                CallPlayCollision(self->vehicle, 0.0f, v, 1);
            }
            self->surface[i] = surface;
        }
    }
}

// 0x58B610 (thiscall): the scooter: idle, engine and turning loops, the horn, and
// accelerating / decelerating sounds on the accelerate button
void __fastcall VehicleAudio_UpdateScooterImpl(VehicleAudio* self, re::Edx) {
    void* vehicle = self->vehicle;
    void* scooter = (vehicle && (At<uint8_t>(vehicle, 0x108) & 7) == 2) ? vehicle : nullptr;
    SoundBanks_Request(soundBanks, self->bank, 5000, 0);
    if (!SoundBanks_IsLoaded(soundBanks, self->bank)) {
        return;
    }
    int idleId = GetId("ScootIdle");
    int engineId = GetId("ScootEngine");
    CVector* pos = EntityPos(scooter);
    CVector cam, d;
    VectorSub(&d, pos, GetCameraPos(&cam));
    float dist = float(std::sqrt(double(std::fabs(d.x * d.x + d.y * d.y + d.z * d.z))));
    float s = std::fabs(ForwardSpeed(scooter)) / 0.38f;
    if (!(s <= 1.0f)) {
        s = 1.0f;
    }
    float sp = s < 0.0f ? 0.0f : s;
    float turn = At<float>(scooter, 0x138) / 0.015f;
    float gasAbs = std::fabs(At<float>(scooter, 0x294));
    float brake = At<float>(scooter, 0x298);
    if (!Seats_IsDriver(Seats(scooter), playerPed)) {
        sp = sp * 8.0f;
        if (1.0f < sp) {
            sp = 1.0f;
        }
    }
    if (turn < -1.0f) {
        turn = -1.0f;
    } else if (turn > 1.0f) {
        turn = 1.0f;
    }
    Audio_ApproachF(&self->lean, turn, 0.2f);
    Audio_ApproachF(&self->turn, sp, 0.1f);
    turn = self->lean;
    sp = self->turn;
    float spAbs = std::fabs(sp);
    int idleVol = 0;
    if (!(0.8f < spAbs)) {
        idleVol = int((1.0f - spAbs) * 1024.0f * 0.8f);
    }
    int idlePitch = int(spAbs * 1000.0f - 500.0f);
    if (Seats_IsDriver(Seats(scooter), playerPed)) {
        if (double(Pad_GetValue(8, 0)) > 0.10000000149011612) {
            int vol = int(Audio_Fn59E4F0(1024.0f, 4));
            SoundSlot_Play597010(&self->sound1C, GetId("ScooterHorn"), EntityPos(scooter), 2, vol, 0, 0, 0);
        }
        if (double(gasAbs) < 0.05000000074505806 && idleVol < 900) {
            idleVol = 900;
        }
        if (self->sound4) {
            int vol = VCall<int>(self->sound4, 0x10);
            if (Audio_Approach(&vol, idleVol, 0x50)) {
                VCall<void>(self->sound4, 0x14, vol);
            }
            idleVol = vol;
        }
    }
    int engineVol;
    if (0.1f > sp) {
        engineVol = int(10.0f * spAbs * 700.0f);
    } else {
        engineVol = int((spAbs - 0.1f) * 324.0f / 0.9f + 700.0f);
    }
    int enginePitch;
    if (0.4f > sp) {
        enginePitch = int(spAbs / 0.4f * 250.0f - 400.0f);
    } else {
        enginePitch = int((spAbs - 0.4f) / 0.6f * 500.0f - 150.0f);
    }
    int engineTarget = engineVol;
    if (Seats_IsDriver(Seats(scooter), playerPed)) {
        if (double(gasAbs) < 0.05000000074505806) {
            if (sp < 0.5f) {
                engineTarget = int(((sp + sp) * 0.65f + 0.35f) * 0.95f * float(engineVol));
            } else {
                engineTarget = int(float(engineVol) * 0.95f);
            }
        }
        if (self->sound8) {
            int vol = VCall<int>(self->sound8, 0x10);
            if (Audio_Approach(&vol, engineTarget, 0x50)) {
                VCall<void>(self->sound8, 0x14, vol);
            }
            engineTarget = vol;
        }
    }
    int turnVol = 0;
    if (double(sp) > 0.10000000149011612) {
        turnVol = int(std::fabs(turn) * 324.0f + 700.0f);
    }
    float turnAbs = std::fabs(turn);
    float sp500 = sp * 500.0f;
    int turnPitch = int(sp500 + turnAbs * 200.0f);
    if (Seats_GetOccupant(Seats(scooter), 0)) {
        if (!self->sound4 && !self->sound8) {
            CVector* at = EntityPos(scooter);
            void* sound = Sounds_PlayAt(TheSounds, GetId("ScootOff"), at, 2, at, 0, 0, 0, 0, nullptr, nullptr, 6, 1, 1.0f, 0);
            Sound_Set(&self->sound18, sound);
        }
        int master = int(Audio_Fn59E4F0(float(SFX_Fn59E3E0(dist, 2)), 4));
        int idleScaled = Scale1024(master, idleVol);
        int engineScaled = Scale1024(master, engineTarget);
        int turnScaled = Scale1024(master, turnVol);
        if (idleId) {
            SoundSlot_Play597010(&self->sound4, idleId, EntityPos(scooter), 2, idleScaled, idlePitch, 0, 1);
        }
        if (engineId) {
            SoundSlot_Play597010(&self->sound8, engineId, EntityPos(scooter), 2, engineScaled, enginePitch, 0, 1);
        }
        if (Seats_IsDriver(Seats(scooter), playerPed) && 0.01f < turnAbs && double(brake) < 0.05000000074505806) {
            int id = GetId(0.0f > turn ? "ScooterTurnL" : "ScooterTurnR");
            SoundSlot_Play597010(&self->sound14, id, EntityPos(scooter), 2, turnScaled, turnPitch, 0, 0);
        }
    }
    if (Seats_IsDriver(Seats(scooter), playerPed)) {
        Pad_JustPressed(6, 0);
        uint8_t accelerate = Pad_JustPressed(7, 0);
        uint8_t released = Pad_JustReleased(7, 0);
        if (accelerate) {
            if (self->sound18) {
                VCall<void>(self->sound18, 0x34);
                Sound_Set(&self->sound18, nullptr);
            }
            int pitch = int(sp500 - 300.0f);
            if (self->soundC) {
                VCall<void>(self->soundC, 0x24, float(pitch));
            } else {
                CVector* at = EntityPos(scooter);
                void* sound = Sounds_PlayAt(TheSounds, GetId("ScootAcel"), at, 2, at, 0, 0, 0, 0, nullptr, nullptr, 6, 0, 1.0f,
                                            pitch);
                Sound_Set(&self->soundC, sound);
                if (self->soundC) {
                    VCall<void>(self->soundC, 0x24, float(pitch));
                    At<uint8_t>(self->soundC, 0x5F) = 1;
                }
            }
        } else if (released) {
            if (self->soundC) {
                VCall<void>(self->soundC, 0x34);
                Sound_Set(&self->soundC, nullptr);
            }
            int pitch = int(sp500);
            if (self->sound18) {
                VCall<void>(self->sound18, 0x24, float(pitch));
            } else {
                if (0.75f > sp) {
                    CVector* at = EntityPos(scooter);
                    void* sound = Sounds_PlayAt(TheSounds, GetId("ScootDecel"), at, 2, at, 0, 0, 0, 0, nullptr, nullptr, 6, 0,
                                                1.0f, pitch);
                    Sound_Construct(&self->sound18, sound);
                }
                if (self->sound18) {
                    VCall<void>(self->sound18, 0x24, float(pitch));
                    At<uint8_t>(self->sound18, 0x5F) = 1;
                }
            }
        }
        VehicleAudio_UpdateTyres(self);
        return;
    }
    if (Seats_GetOccupant(Seats(scooter), 0) || Seats_GetOccupant(Seats(scooter), 0)) {
        return;
    }
    if (!self->sound4 && !self->sound8) {
        if (!self->sound18) {
            At<uint8_t>(scooter, 0x33A) &= 0xFD;
        }
        return;
    }
    CVector* at = EntityPos(scooter);
    void* sound = Sounds_PlayAt(TheSounds, GetId("ScootOff"), at, 2, at, 0, 0, 0, 0, nullptr, nullptr, 6, 1, 1.0f, 0);
    Sound_Set(&self->sound18, sound);
}

// 0x58BF90 (thiscall): the update by the vehicle's class (mower, go-kart and scooter
// their own); other cars' engines by their distance change, or with their own bank the
// bulldozer's engine and the engine-off sounds of empty ones
void __fastcall VehicleAudio_UpdateImpl(VehicleAudio* self, re::Edx) {
    void* vehicle = self->vehicle;
    if (!vehicle || (At<uint8_t>(vehicle, 0x108) & 7) != 2) {
        return;
    }
    switch (VehicleClass(vehicle)) {
    case 6:
        VehicleAudio_UpdateSmallEngine(self);
        return;
    case 7:
        VehicleAudio_UpdateGoKart(self);
        return;
    case 8:
        VehicleAudio_UpdateScooter(self);
        return;
    }
    if (VehicleClass(vehicle) != 7) {
        SoundBanks_Request(soundBanks, SoundBanks_Find(soundBanks, "Engine.bnk"), 5000, 0);
    }
    CVector cam, d;
    GetCameraPos(&cam);
    VectorSub(&d, EntityPos(vehicle), &cam);
    float dist = Vector_Magnitude(&d);
    float delta = self->distance - dist;
    self->distance = dist;
    if (self->bank == -1) {
        if (VehicleClass(self->vehicle) == 10) {
            VehicleAudio_UpdateArcade(self);
        } else {
            VehicleAudio_UpdateEngine(self, delta);
        }
        return;
    }
    SoundBanks_Request(soundBanks, self->bank, 5000, 0);
    self->bankLoaded = SoundBanks_IsLoaded(soundBanks, self->bank);
    if (!self->bankLoaded) {
        return;
    }
    if (Seats_IsDriver(Seats(vehicle), playerPed) || Seats_GetOccupant(Seats(vehicle), 0)) {
        return;
    }
    if (At<int16_t>(vehicle, 0x10E) == 0xBE) {
        if (self->sound8) {
            return;
        }
        CVector* pos = EntityPos(self->vehicle);
        void* sound = Sounds_PlayAt(TheSounds, GetId("DozerEngine"), pos, 2, pos, 0, 0x7D, 0x7E, 0x7F,
                                    reinterpret_cast<void*>(kDozerCallback), self->vehicle, 6, 1, 1.0f, 0);
        Sound_Construct(&self->sound8, sound);
        if (self->sound8) {
            At<VehicleAudio*>(self->sound8, 0x68) = self;
            VCall<void>(self->sound8, 0x14, 0);
            return;
        }
        VehicleAudios_Remove(vehicleAudios, vehicle);
        return;
    }
    bool stopped = false;
    if (self->sound4) {
        CutSound(&self->sound4);
        stopped = true;
    }
    if (self->sound8) {
        CutSound(&self->sound8);
        stopped = true;
    }
    if (self->sound10) {
        CutSound(&self->sound10);
    } else if (!stopped) {
        if (VehicleClass(vehicle) == 7 && !self->sound14) {
            At<uint8_t>(vehicle, 0x33A) &= 0xFD;
        }
        return;
    }
    if (VehicleClass(vehicle) == 7) {
        void* sound = Sounds_PlayAt(TheSounds, GetId("K1_EngStop"), EntityPos(self->vehicle), 1, nullptr, 0, 0x78, 0, 0,
                                    nullptr, nullptr, 6, 1, 1.0f, 0);
        Sound_Construct(&self->sound14, sound);
    } else if (VehicleClass(vehicle) == 8) {
        Sounds_PlayAt(TheSounds, GetId("ScootOff"), EntityPos(self->vehicle), 1, nullptr, 0, 0x78, 0, 0, nullptr, nullptr, 6,
                      1, 1.0f, 0);
    }
}

// ---- the manager -----------------------------------------------------------------------

// 0x58C360 (thiscall): every vehicle's sounds updated (freed ones reset)
void __fastcall VehicleAudios_Update(VehicleAudioManager* self, re::Edx) {
    for (int i = 0; i < self->count; i++) {
        VehicleAudio* e = self->entries[i];
        if (!e) {
            continue;
        }
        if (e->vehicle) {
            VehicleAudio_Update(e);
        } else if (e->index > -1) {
            VehicleAudio_Reset(e);
        }
    }
}

// 0x58C3A0 (thiscall): the constructor (its entries allocated)
VehicleAudioManager* __fastcall VehicleAudios_Construct(VehicleAudioManager* self, re::Edx, int count, int unknown) {
    self->count = count;
    self->unknown24 = unknown;
    for (int i = 0; i < count; i++) {
        void* p = Malloc(0x74);
        self->entries[i] = p ? VehicleAudio_Construct(static_cast<VehicleAudio*>(p)) : nullptr;
    }
    self->freeHint = 0;
    return self;
}

// 0x58C400 (thiscall): every used entry reset
void __fastcall VehicleAudios_ResetAll(VehicleAudioManager* self, re::Edx) {
    for (int i = 0; i < self->count; i++) {
        VehicleAudio* e = self->entries[i];
        if (e && e->index > -1) {
            VehicleAudio_Reset(e);
        }
    }
}

// 0x58C430 (thiscall): a vehicle's entry reset (and hinted free)
void __fastcall VehicleAudios_RemoveImpl(VehicleAudioManager* self, re::Edx, void* vehicle) {
    if (!vehicle) {
        return;
    }
    for (int i = 0; i < self->count; i++) {
        VehicleAudio* e = self->entries[i];
        if (e && e->vehicle == vehicle) {
            if (i > -1) {
                VehicleAudio_Reset(e);
                self->freeHint = i;
            }
            return;
        }
    }
}

// 0x58C470 (thiscall): the entries deleted
void __fastcall VehicleAudios_Shutdown(VehicleAudioManager* self, re::Edx) {
    for (int i = 0; i < self->count; i++) {
        VehicleAudio* e = self->entries[i];
        if (e) {
            VehicleAudio_Destruct(e);
            Free(e);
        }
        self->entries[i] = nullptr;
    }
}

// 0x58C4B0 (thiscall): a vehicle given a free entry unless it has one
void __fastcall VehicleAudios_Add(VehicleAudioManager* self, re::Edx, void* vehicle) {
    for (int i = 0; i < self->count; i++) {
        if (self->entries[i]->vehicle == vehicle) {
            return;
        }
    }
    int index = VehicleAudios_FindFree(self);
    if (index > -1) {
        CallInit(self->entries[index], vehicle, index);
    }
}

} // namespace

#define VAUDIO2_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(vaudio2_original_, __LINE__);                                      \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(vaudio2_original_, __LINE__), \
             re::Mode::Original)

VAUDIO2_REVERSED("VehicleAudio::UpdateTyres", 0x0058AB90, &VehicleAudio_UpdateTyresImpl);
VAUDIO2_REVERSED("VehicleAudio::UpdateGoKart", 0x0058AE70, &VehicleAudio_UpdateGoKartImpl);
VAUDIO2_REVERSED("VehicleAudio::UpdateScooter", 0x0058B610, &VehicleAudio_UpdateScooterImpl);
VAUDIO2_REVERSED("VehicleAudio::Update", 0x0058BF90, &VehicleAudio_UpdateImpl);
VAUDIO2_REVERSED("VehicleAudios::Update", 0x0058C360, &VehicleAudios_Update);
VAUDIO2_REVERSED("VehicleAudios::VehicleAudios", 0x0058C3A0, &VehicleAudios_Construct);
VAUDIO2_REVERSED("VehicleAudios::ResetAll", 0x0058C400, &VehicleAudios_ResetAll);
VAUDIO2_REVERSED("VehicleAudios::Remove", 0x0058C430, &VehicleAudios_RemoveImpl);
VAUDIO2_REVERSED("VehicleAudios::Shutdown", 0x0058C470, &VehicleAudios_Shutdown);
VAUDIO2_REVERSED("VehicleAudios::Add", 0x0058C4B0, &VehicleAudios_Add);
