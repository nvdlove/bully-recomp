// CBike (vtable 0x912B24): its destructor, its sounds and skid marks, its animations
// on crashing, its steering, and the swinging parts (a spring: 0x4B9460..0x4B94E0).
// Fields are reached by offset: CBike's layout (Vehicle.h) only has a few named.

#include "game/Vehicle.h"

#include <cmath>

#include "game/Crt.h"
#include "reversed.h"
namespace {

using veh::CVector;

using re::At;

// a virtual by its byte offset in the vtable
template <typename R, typename... A>
R Virtual(void* object, int offset, A... args) {
    re::Ptr32<void>* vtable = *reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(object);
    return reinterpret_cast<R(__thiscall*)(void*, re::Arg<A>...)>(static_cast<void*>(vtable[offset / 4]))(object, args...);
}

GAME_VAR(float, timeStep, 0x00C1A9A4);        // CTimer's
GAME_VAR(CPed*, playerPed, 0x00C1AEA8);       // (FindPlayerPed's)
GAME_VAR(CVector, vec0, 0x00BCC564);          // (a zero vector, also "unset")
GAME_VAR(uint8_t, flagC1AE6F, 0x00C1AE6F);
GAME_VAR(uint8_t*, bikeTuning, 0x020C8900);   // (bike tuning values: +0x28, +0x7C..+0x88)
GAME_VAR(uint8_t*, globals20C88B8, 0x020C88B8);

inline int __cdecl Rand() { return crt::Rand(); }

// the seats (+0x1C4)
GAME_FN(CPed*, __thiscall, Seats_GetOccupant, 0x004CE560, CVehicleSeats* seats, uint32_t seat);
GAME_FN(void, __thiscall, Seats_RemoveDriver, 0x004CE960, CVehicleSeats* seats);
GAME_FN(int32_t*, __thiscall, Vehicle_GetVehicleClass, 0x004CAF30, CVehicle* vehicle, int32_t* out);
GAME_FN(void, __thiscall, Vehicle_Destruct, 0x004CD130, CVehicle* vehicle);
GAME_FN(CVector*, __thiscall, Vehicle_4CCC00, 0x004CCC00, CVehicle* vehicle, CVector* out);
GAME_FN(float, __thiscall, Physical_GetSpeed, 0x0046A580, CVehicle* vehicle);   // (the move speed's length)
GAME_FN(void, __cdecl, Fn_41EE50, 0x0041EE50, CVehicle* vehicle);
GAME_FN(uint8_t, __thiscall, Ped_4765C0, 0x004765C0, CPed* ped);
GAME_FN(void, __thiscall, Ped_472CC0, 0x00472CC0, void* pedPart, int arg);
GAME_FN(void, __cdecl, CleanUpOldReference, 0x004657E0, void* entity, void* ref);
GAME_FN(void, __thiscall, SetRef, 0x005D7F10, void* ref, void* entity);
GAME_FN(CVector*, __thiscall, Physical_GetUp, 0x00473730, CVehicle* vehicle, CVector* out);   // (its matrix's +0x20 row)
GAME_FN(CVector*, __cdecl, VectorAdd, 0x004129A0, CVector* out, const CVector* a, const CVector* b);
GAME_FN(float, __thiscall, Vector_MagnitudeSqr, 0x0040D9F0, const CVector* v);
GAME_FN(void, __thiscall, Physical_468DF0, 0x00468DF0, CVehicle* vehicle);
GAME_FN(float*, __thiscall, Entity_GetMatrix, 0x008B6120, CVehicle* entity);
GAME_FN(void, __thiscall, Matrix_413220, 0x00413220, float* matrix, float angle);
GAME_FN(uint8_t, __thiscall, Seats_IsDriver, 0x004CEA70, CVehicleSeats* seats, CPed* ped);
GAME_FN(CVehicle*, __cdecl, FindPlayerVehicle, 0x00446970, uintptr_t player);
GAME_FN(CVector*, __thiscall, Placement_GetRight, 0x00473600, void* placement, CVector* out);   // (the transform's, without a matrix)
GAME_FN(CVector*, __thiscall, Entity_4154A0, 0x004154A0, CVehicle* entity, CVector* out);
GAME_FN(void, __thiscall, Physical_ApplyTurnForce, 0x0046A950, CVehicle* physical, float fx, float fy, float fz, float px,
        float py, float pz);
GAME_FN(uint8_t, __thiscall, Ped_IsSeatedInVehicle, 0x004CE690, CPed* ped);
GAME_FN(float, __cdecl, Fn_738840, 0x00738840, int a, int b);     // (a tuning float by index)
GAME_FN(uint8_t, __cdecl, Fn_738900, 0x00738900, int a, int b);   // (a tuning flag by index)
GAME_FN(uint8_t, __thiscall, Input_5F4800, 0x005F4800, void* input, void* control, int a, int b);
GAME_FN(CVector*, __cdecl, Normalised, 0x00472D40, CVector* out, const CVector* v);
GAME_FN(float, __cdecl, DotProduct, 0x00414640, const CVector* a, const CVector* b);
GAME_FN(CVector*, __cdecl, VectorScale, 0x00412350, CVector* out, const CVector* v, float s);
GAME_FN(CVector*, __cdecl, VectorSub, 0x00414B70, CVector* out, const CVector* a, const CVector* b);
GAME_FN(void, __thiscall, Physical_ApplyMoveForce, 0x0045D3E0, CVehicle* physical, const CVector* force);
GAME_FN(void, __thiscall, Physical_45D410, 0x0045D410, CVehicle* physical, const CVector* a, const CVector* b);   // (to ApplyTurnForce)
GAME_VAR(uint32_t, timeInMs, 0x00C1A9B4);     // CTimer's
inline void* const controlBikeJump = reinterpret_cast<void*>(0x01B653EC);   // (a control, for 0x5F4800)
GAME_FN(void*, __cdecl, Fn_738070, 0x00738070);
GAME_FN(uint8_t, __cdecl, Fn_738500, 0x00738500, void* arg);
GAME_VAR(uint8_t, flagA14925, 0x00A14925);
inline void* const objA45A30 = reinterpret_cast<void*>(0x00A45A30);
GAME_FN(uint8_t, __thiscall, Fn_50DF50, 0x0050DF50, void* object);
GAME_FN(float, __cdecl, RandomFloatInRange, 0x0041A5B0, float lo, float hi);
GAME_FN(float, __thiscall, Vector_Magnitude, 0x0040DFB0, const CVector* v);
GAME_VAR(int32_t, currArea, 0x00BD1008);       // CGame's
GAME_VAR(uint8_t, chapter, 0x00A147E0);
GAME_ARRAY(int32_t, chapterAreaKinds, 0x00A147EC);   // (by chapter: 2 the snowy one?)
GAME_FN(uint8_t, __cdecl, Fn_430320, 0x00430320, int area);
GAME_FN(void, __thiscall, SoundManager_Play, 0x00668290, void* manager, uint32_t* id, void* sound, int arg);
GAME_FN(void, __thiscall, Sound_SetPosition, 0x00669040, void* sound, const CVector* pos, int arg);
GAME_FN(void, __thiscall, Sound_SetVolume_x64, 0x00668C40, void* sound, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Sound_SetVolume(void* a0, float a1) {
    return Sound_SetVolume_x64(a0, re::BitsOf(a1));
}
GAME_FN(void, __thiscall, Sound_SetPitch_x64, 0x00668C10, void* sound, uint32_t);   // (x64: the body's vector shape, tools/x64_vector_args.py)
inline void Sound_SetPitch(void* a0, float a1) {
    return Sound_SetPitch_x64(a0, re::BitsOf(a1));
}
// this file's own, called where they are (so bullyre.ini decides which runs)
GAME_FN(void, __thiscall, Game_SetTrails, 0x004B95C0, CBike* bike, int on);
GAME_FN(void, __cdecl, Game_LoadSounds, 0x004B9B00);
GAME_FN(void, __thiscall, Game_StopSounds, 0x004B9BA0, CBike* bike);
GAME_FN(CPed*, __thiscall, Game_KnockOffRider, 0x004BA050, CBike* bike, int kind, const void* arg, CPed* ped, int unused);
GAME_FN(void, __thiscall, Game_GetSurfaceSound, 0x004BBA40, CBike* bike, int32_t* out);
GAME_VAR(float, staticMoveSpeed, 0x00A35714);    // (per step: below it the bike counts towards static)
GAME_VAR(float, staticTurnSpeed, 0x00A35718);
GAME_VAR(uint32_t, staticFrames, 0x00A3571C);    // (the count that makes it static)

// the skid marks / trails (0x1B850E0)
inline void* const trails = reinterpret_cast<void*>(0x01B850E0);
GAME_FN(int32_t, __thiscall, Trails_Add, 0x0066BB00, void* trails, int kind, float width, uint32_t colour, int arg);
GAME_FN(void, __thiscall, Trails_Remove, 0x0066BF40, void* trails, int id);

// the effects (+0x6F0, +0x6F8, +0x718) and the sounds' bank (0xB6F1E8)
GAME_VAR(void*, soundBank, 0x00B6F1E8);
GAME_VAR(void*, soundManager, 0x00B6F1D4);
GAME_FN(uint16_t, __thiscall, SoundBank_GetId, 0x00667B60, void* bank, const char* name);
GAME_FN(void, __thiscall, Sound_Stop, 0x00668D00, void* sound, float fade);
GAME_FN(void, __thiscall, SoundManager_Release, 0x00668100, void* manager, void* sound);
GAME_FN(void, __thiscall, Effect_Reset, 0x00668BA0, void* effect);
GAME_FN(void, __thiscall, Effect_Destruct, 0x00668EB0, void* effect);

// the bike sounds' ids, found once
GAME_VAR(int32_t, soundBikeOnGrass, 0x00A35720);   // -1 until found
GAME_VAR(int32_t, soundBikeOnSand, 0x00A35724);
GAME_VAR(int32_t, soundBikeOnDirt, 0x00A35728);
GAME_VAR(int32_t, soundBikeOnBigWet, 0x00A3572C);
GAME_VAR(int32_t, soundBikeBrake, 0x00A35730);
GAME_VAR(int32_t, soundBikeBrakeSnow, 0x00A35734);

// animations
inline void* const animManager = reinterpret_cast<void*>(0x020C4910);
GAME_FN(int, __cdecl, AnimGroupIndex, 0x00576ED0, uintptr_t group);   // (CBaseModelInfo's; cdecl, the 0 pushed before it stays)
GAME_FN(void*, __thiscall, AnimGroups_Find, 0x00465580, void* groups, int index, int32_t* outIndex);
GAME_FN(void, __thiscall, AnimManager_6C1190, 0x006C1190, void* manager, void* animData, int arg, uintptr_t anim, void* group);
GAME_FN(int, __thiscall, AnimDict_Find, 0x006B1D60, void* dict, int arg, const char* name, re::Ptr32<void>* out);
GAME_FN(void*, __thiscall, AnimData_6B1F50, 0x006B1F50, void* animData, int arg);
GAME_FN(void, __thiscall, AnimManager_6C0D40, 0x006C0D40, void* manager, void* animData, void* anim, void* slot, int arg,
        float blend, float speed);

// the type ids (0xBF3830 counts them out, each class takes the next the first time)
GAME_VAR(uint16_t, nextTypeId, 0x00BF3830);
GAME_VAR(uint32_t, bikeTypeIdGuard, 0x00C2D19C);
GAME_VAR(uint16_t, bikeTypeId, 0x00C2D198);
GAME_VAR(uint32_t, vehicleTypeIdGuard, 0x00C0F6A4);
GAME_VAR(uint16_t, vehicleTypeId, 0x00C0F6A0);

inline void* const bikeVtable = reinterpret_cast<void*>(0x00912B24);

CVehicleSeats* Seats(void* bike) {
    return &static_cast<CVehicle*>(bike)->seats;
}

// ---- the spring (a swinging part: +0x20 its angle, +0x28 its angular speed) ----------

// 0x4B9460 (thiscall): at rest (eax is left `this`)
void* __fastcall Spring_Stop(uint8_t* self) {
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x28) = 0.0f;
    return self;
}

// 0x4B9470 (thiscall): at rest, then a random push (+8 times -+0xC..+0xC)
void __fastcall Spring_Reset(uint8_t* self) {
    At<float>(self, 0x20) = 0.0f;
    At<float>(self, 0x28) = 0.0f;
    At<int32_t>(self, 0x1C) = 0;
    At<float>(self, 0x24) = At<float>(self, 0x10);
    float lo = -At<float>(self, 0xC);
    float hi = At<float>(self, 0xC);
    float r = float(double(Rand()) / 32767.0 * (hi - lo) + lo);
    At<float>(self, 0x28) = At<float>(self, 8) * r + At<float>(self, 0x28);
}

// 0x4B94E0 (thiscall): one step; +4 the stiffness grows by +0x18 a step up to +0x14
void __fastcall Spring_Update(uint8_t* self, re::Edx, float scale) {
    float oldAngle = At<float>(self, 0x20);
    float t = float(std::tan(double(At<float>(self, 0x20))));
    At<float>(self, 0x28) = t * At<float>(self, 4) + At<float>(self, 0x28);
    float stiffness = At<float>(self, 0x18) * timeStep + At<float>(self, 4);
    At<float>(self, 4) = stiffness;
    float max = At<float>(self, 0x14);
    if (!(max <= stiffness)) {   // (the smaller of the two)
        max = stiffness;
    }
    At<float>(self, 4) = max;
    float angle = At<float>(globals20C88B8, 0xA8) * scale * max * timeStep + At<float>(self, 0x20);
    At<float>(self, 0x20) = angle;
    angle = At<float>(self, 0x28) * timeStep + angle;
    At<float>(self, 0x20) = angle;
    At<float>(self, 0x28) = (1.0f / timeStep) * (angle - oldAngle);
}

// ---- CBike --------------------------------------------------------------------------

// 0x4B95C0: its two trails (skid marks) started (a new pair) or stopped
void __fastcall Bike_SetTrails(CBike* self, re::Edx, int skidding) {
    int32_t& left = At<int32_t>(self, 0x700);
    int32_t& right = At<int32_t>(self, 0x704);
    uint8_t& on = At<uint8_t>(self, 0x708);
    if (!re::Flag(skidding)) {
        if (on) {
            if (left != -1) {
                Trails_Remove(trails, left);
            }
            if (right != -1) {
                Trails_Remove(trails, right);
            }
            left = -1;
            right = -1;
            on = 0;
            At<uint8_t>(self, 0x709) = 0;
        }
        return;
    }
    if (on) {
        return;
    }
    if (left != -1) {
        Trails_Remove(trails, left);
    }
    if (right != -1) {
        Trails_Remove(trails, right);
    }
    left = Trails_Add(trails, 5, 0.9f, 0xA0C0C0C0, 2);
    right = Trails_Add(trails, 5, 0.9f, 0xA0C0C0C0, 2);
    on = 1;
    At<uint8_t>(self, 0x709) = 0;
}

// 0x4B96C0: flags33B 0x40 while it's touching something; 0 (and +0x40 = 1) when one of
// the two is something with +0x38 while 0xC1AE6F is set
uint8_t __fastcall Bike_4B96C0(CBike* self) {
    self->flags33B &= 0xBF;
    for (int i = 0; i < 2; i++) {
        CEntity* entity = self->entities[i];
        if (!entity) {
            continue;
        }
        self->flags33B |= 0x40;
        if (flagC1AE6F && At<int32_t>(entity, 0x38) != 0) {
            At<int32_t>(self, 0x40) = 1;
            return 0;
        }
    }
    if (self->flags33B & 0x40) {
        At<uint8_t>(self, 0x6B8) &= 0xDF;
        self->unknown180 = 0;
    }
    return 1;
}

// 0x4B9740 (two arguments, unused): stopped (moving, when +0x1BC has 4)
void __fastcall Bike_4B9740(CBike* self, re::Edx, int, int) {
    self->flags33B &= 0xF5;
    if (!(self->flags33A & 0x10)) {
        Fn_41EE50(self);
    }
    if (At<uint8_t>(self, 0x1BC) & 4) {
        self->moveSpeed = vec0;
        self->turnSpeed = vec0;
    }
    At<float>(self, 0x294) = float(double(self->unknown29C) * 0.0);   // (keeps a NaN)
    At<float>(self, 0x298) = 0.0f;
}

// 0x4B9800 (thiscall, CBike's): steering towards 0x4CCC00's direction, within the
// handling's steering lock (+0x9C degrees); straight when the driver's +0x2E0 is unset
void __fastcall Bike_UpdateSteering(CBike* self) {
    if (Seats_GetOccupant(Seats(self), 0)) {
        const CVector* v = At<CVector*>(Seats_GetOccupant(Seats(self), 0), 0x2E0);
        if (vec0.x == v->x && vec0.y == v->y && vec0.z == v->z) {
            Virtual<void>(self, 0xEC, vec0.x, vec0.y, vec0.z);   // (vf59 takes the floats)
            Virtual<void>(self, 0xE0, 0.0f);
            return;
        }
    }
    self->flags33A &= 0xFB;
    CVector dir;
    Vehicle_4CCC00(self, &dir);
    Virtual<void>(self, 0xEC, dir.x, dir.y, dir.z);   // (vf59 takes the floats)
    float lock = float(double(At<float>(self->handling, 0x9C)) * 3.1415927410125732 / 180.0);
    float heading = float(std::atan2(double(-dir.x), double(dir.y)));
    float steer = float(double(heading) * 1.100000023841858);
    float scale = 1.0f;
    float absSteer = std::fabs(steer);
    if (lock > absSteer) {
        scale = float((lock - absSteer) / lock * 1.2000000476837158 + 1.0);
    }
    float m = self->unknown29C;
    if (!(m > scale)) {   // (the smaller of the two)
        scale = m;
    }
    if (scale < 0.0) {
        scale = 0.0f;
    }
    self->unknown29C = scale;
    float clamped;
    if (-lock > steer) {
        clamped = -lock;
    } else if (steer > lock) {
        clamped = lock;
    } else {
        clamped = steer;
    }
    if (0.0f > At<float>(self, 0x294) && 0.0f > dir.y) {
        clamped = float(double(clamped) * -1.0);
    }
    Virtual<void>(self, 0xE0, clamped);
}

// vf57 0x4B9A30: from its speed in km/h (the tuning's +0x80..+0x88 map to +0x7C..+0x84)
float __fastcall Bike_GetUnknownVf57(CBike* self) {
    float kmh = float(double(Physical_GetSpeed(self)) * 3.5999999046325684);
    uint8_t* t = bikeTuning;
    if (At<float>(t, 0x80) > kmh) {
        return At<float>(t, 0x7C);
    }
    if (At<float>(t, 0x88) < kmh) {
        return At<float>(t, 0x84);
    }
    float range = At<float>(t, 0x84) - At<float>(t, 0x7C);
    // (the range divided and multiplied back: likely meant the speeds' range)
    return range * ((kmh - At<float>(t, 0x80)) / range);
}

// vf55 0x4B9AB0: the horn's random honks (+0x36E counts down, +0x368 the honk's length)
void __fastcall Bike_UpdateHorn(CBike* self) {
    if (self->unknown368 != 0) {
        return;
    }
    if (self->unknown36E) {
        self->unknown36E--;
        return;
    }
    uint8_t r = uint8_t((uint8_t(Rand()) & 0x7F) + 0x96);
    self->unknown36E = r;
    if ((r & 7) < 4) {
        self->unknown368 = 0x2D;
    }
}

// 0x4B9B00 (static): the bike sounds' ids (the surfaces' once)
void __cdecl Bike_LoadSounds() {
    if (soundBikeOnGrass == -1) {
        soundBikeOnGrass = SoundBank_GetId(soundBank, "BikeOnGrass");
        soundBikeOnSand = SoundBank_GetId(soundBank, "BikeOnSand");
        soundBikeOnDirt = SoundBank_GetId(soundBank, "BikeOnDirt");
        soundBikeOnBigWet = SoundBank_GetId(soundBank, "BikeOnBigWet");
    }
    soundBikeBrake = SoundBank_GetId(soundBank, "BikeBrake");
    soundBikeBrakeSnow = SoundBank_GetId(soundBank, "BikeBrakeSnow");
}

// 0x4B9BA0: its two sounds (+0x6F0, +0x6F8) faded out and let go
void __fastcall Bike_StopSounds(CBike* self) {
    static const int offsets[] = {0x6F0, 0x6F8};
    for (int off : offsets) {
        if (At<int32_t>(self, off + 4) != 0) {
            void* sound = &At<uint8_t>(self, off);
            Sound_Stop(sound, 0.5f);
            if (At<int32_t>(self, off + 4) != 0) {
                SoundManager_Release(soundManager, sound);
            }
        }
    }
}

// 0x4B9C30: the top speed: the handling's (+0x84), the player's own (+0x69C), scaled
// by +0x778 between the tuning's +0x28 and 1
float __fastcall Bike_GetTopSpeed(CBike* self) {
    float top = At<float>(self->handling, 0x84);
    if (Seats_GetOccupant(Seats(self), 0) && Seats_GetOccupant(Seats(self), 0) == playerPed) {
        top = At<float>(self, 0x69C);
    }
    float min = At<float>(bikeTuning, 0x28);
    return ((1.0f - min) * self->unknown778 + min) * top;
}

// 0x4B9CE0: an animation of a group on its animation data
void __fastcall Bike_PlayAnim(CBike* self, re::Edx, int group, int anim) {
    void* animData = self->animData;
    if (!animData) {
        return;
    }
    void* groups = At<void*>(animData, 0x94);
    if (!groups) {
        return;
    }
    void* found = AnimGroups_Find(groups, AnimGroupIndex(group), nullptr);
    if (found) {
        AnimManager_6C1190(animManager, self->animData, -1, anim, found);
    }
}

void PlayBaseAnim(CBike* self, const char* name) {
    re::Ptr32<void> anim;
    void* dict = At<void*>(At<void*>(self->animData, 0x94), 0x10);
    if (!AnimDict_Find(dict, 0, name, &anim)) {
        anim = nullptr;
    }
    void* slot = AnimData_6B1F50(self->animData, 0);
    AnimManager_6C0D40(animManager, self->animData, anim, slot, 2, 1.0f, -1.0f);
}

// 0x4B9D40: its base pose (the police bike's, the scooter's, or the plain one)
void __fastcall Bike_PlayBaseAnim(CBike* self) {
    int32_t cls;
    const char* name;
    if (*Vehicle_GetVehicleClass(self, &cls) == 3) {
        name = "PROPS\\COPBIKE_BASE";
    } else if (*Vehicle_GetVehicleClass(self, &cls) == 8) {
        name = "PROPS\\SCOOTER_BASE";
    } else {
        name = "PROPS\\BASE";
    }
    PlayBaseAnim(self, name);
}

// 0x4B9E10: a wheel (+0x668, 12 bytes each) tilted past -pi/8 from +0x5A0, or no driver
int32_t __fastcall Bike_4B9E10(CBike* self, re::Edx, int wheel) {
    const float* v = &At<float>(self, 0x668 + wheel * 0xC);
    float dot = v[1] * At<float>(self, 0x5A4) + v[0] * At<float>(self, 0x5A0) + v[2] * At<float>(self, 0x5A8);
    if (!(dot < -0.39269909262657166) && !Seats_GetOccupant(Seats(self), 0)) {
        return 0;
    }
    return 1;
}

// 0x4B9E80: the destructor's body (a driver who isn't the player out, and deleted when
// 0x4765C0 says; its effects, sounds and skid marks; its references)
void __fastcall Bike_Destruct(CBike* self) {
    self->vtable = static_cast<re::Ptr32<void>*>(bikeVtable);
    CVehicleSeats* seats = Seats(self);
    if (Seats_GetOccupant(seats, 0) && Seats_GetOccupant(seats, 0) != playerPed) {
        CPed* driver = Seats_GetOccupant(seats, 0);
        Seats_RemoveDriver(seats);
        if (Ped_4765C0(driver) && driver) {
            Virtual<void>(driver, 0, 1);
        }
    }
    Effect_Reset(&At<uint8_t>(self, 0x718));
    Game_StopSounds(self);
    // (the skid marks removed only when they're -1: the id passed is that -1)
    if (At<int32_t>(self, 0x700) == -1) {
        Trails_Remove(trails, -1);
    }
    if (At<int32_t>(self, 0x704) == -1) {
        Trails_Remove(trails, -1);
    }
    Effect_Destruct(&At<uint8_t>(self, 0x718));
    Effect_Destruct(&At<uint8_t>(self, 0x6F8));
    Effect_Destruct(&At<uint8_t>(self, 0x6F0));
    SetRef(&At<uint8_t>(self, 0x6BC), nullptr);
    for (int offset = 0x610; offset >= 0x5E8; offset -= 0x28) {
        if (At<void*>(self, offset)) {
            CleanUpOldReference(At<void*>(self, offset), &At<uint8_t>(self, offset));
        }
    }
    Vehicle_Destruct(self);
}

// vf1 0x4B9F80: is it of this type (a bike's, or a vehicle's) (the ids handed out on
// first use)
uint8_t __fastcall Bike_IsA(CBike*, re::Edx, int type) {
    if (!(bikeTypeIdGuard & 1)) {
        bikeTypeIdGuard |= 1;
        bikeTypeId = nextTypeId++;
    }
    if (int16_t(type) == int16_t(bikeTypeId)) {
        return 1;
    }
    if (!(vehicleTypeIdGuard & 1)) {
        vehicleTypeIdGuard |= 1;
        vehicleTypeId = nextTypeId++;
    }
    return int16_t(type) == int16_t(vehicleTypeId);
}

// 0x4BA050: the rider knocked off (not off a police bike): the base pose, the rider's
// fall (the damage kind -6 a crash, -5 hit by a car with the car's handling +0xC8,
// -3 also 0x472CC0(5)); returns the ped (the fourth argument unused)
CPed* __fastcall Bike_KnockOffRider(CBike* self, re::Edx, int kind, const void* arg, CPed* ped, int) {
    int32_t cls;
    if (*Vehicle_GetVehicleClass(self, &cls) == 3) {
        return ped;
    }
    PlayBaseAnim(self, "PROPS\\BASE");
    if (!ped) {
        return ped;
    }
    // (CPed's vf15 CPed_ApplyHit: the attacker, the direction and the node source pointers, full width)
    using Fall = void(__thiscall*)(CPed*, float, float, uintptr_t, int, const void*, uintptr_t, uintptr_t, int, const char*, int, float,
                                   float, int, int, int, int, float, int);
    Fall fall = re::PtrCast<Fall>((*reinterpret_cast<re::Ptr32<re::Ptr32<void>>*>(ped))[0x3C / 4]);
    if (kind == -6) {
        fall(ped, 0.01f, 0.0f, 0, -1, arg, 0, 0, 0, "bikecrash", 0, 0.0f, 0.0f, 0, 1, 0, -1, 0.0f, 0);
        return ped;
    }
    if (kind == -5) {
        CVehicle* by = re::PtrCast<CVehicle*>(self->damageEntity);
        if (by && (by->typeAndStatus & 7) == 2 && by->vehicleType == 0) {
            fall(ped, At<float>(by->handling, 0xC8), 0.0f, 0, -1, arg, 0, 0, 0, nullptr, 0, 0.0f, 0.0f, 0, 1, 0, -1, 0.0f, 0);
            return ped;
        }
    }
    fall(ped, 0.01f, 0.0f, 0, -1, arg, 0, 0, 0, nullptr, 0, 0.0f, 0.0f, 0, 1, 0, -1, 0.0f, 0);
    if (kind == -3) {
        Ped_472CC0(At<uint8_t*>(ped, 0x2E0) + 0x408, 5);
    }
    return ped;
}


// 0x4BA220: GTA's "static" check: while it's a parked or abandoned bike (status 4 or 5)
// nobody rides, at rest (its averaged speeds low, upright, +0x634/+0x638 not 1) its
// +0x180 counts up by 2 (down by 1 otherwise), and past 0xA3571C (or already at rest,
// not moving at all) it's made static with no speed
void __fastcall Bike_CheckStatic(CBike* self) {
    At<uint8_t>(self, 0x6B8) &= 0xDF;
    if (At<int32_t>(self, 0x34) != 0) {
        return;
    }
    uint8_t status = self->typeAndStatus >> 3;
    if (status != 4 && status != 5) {
        return;
    }
    if (At<void*>(self, 0x6BC) != nullptr) {
        return;
    }
    if (Seats_GetOccupant(Seats(self), 0)) {
        return;
    }
    bool still = false;
    CVector tmp;
    if (!(self->flags33B & 2) && self->moveSpeed.x == 0.0f && self->moveSpeed.y == 0.0f && self->moveSpeed.z == 0.0f &&
        At<float>(self, 0x634) != 1.0f && At<float>(self, 0x638) != 1.0f && 0.0f < Physical_GetUp(self, &tmp)->z) {
        still = true;
    }
    CVector& moveAvg = At<CVector>(self, 0x13C);
    CVector& turnAvg = At<CVector>(self, 0x148);
    const CVector* sum = VectorAdd(&tmp, &moveAvg, &self->moveSpeed);
    moveAvg = {float(sum->x * 0.5), float(sum->y * 0.5), float(sum->z * 0.5)};
    sum = VectorAdd(&tmp, &turnAvg, &self->turnSpeed);
    turnAvg = {float(sum->x * 0.5), float(sum->y * 0.5), float(sum->z * 0.5)};
    float moveLimit = timeStep * staticMoveSpeed;
    float moveSqr = moveAvg.y * moveAvg.y + moveAvg.x * moveAvg.x + moveAvg.z * moveAvg.z;
    bool atRest = false;
    if (!(float(moveLimit * moveLimit) < moveSqr)) {
        double turnLimit = float(timeStep * staticTurnSpeed);
        float turnSqr = Vector_MagnitudeSqr(&turnAvg);
        if (!(float(turnLimit * turnLimit) < turnSqr) && 0.005f > At<float>(self, 0x19C) &&
            0.0f < Physical_GetUp(self, &tmp)->z && At<float>(self, 0x634) != 1.0f && At<float>(self, 0x638) != 1.0f) {
            atRest = true;
        }
    }
    if (!atRest && !still) {
        if (self->unknown180) {
            self->unknown180--;
        }
        return;
    }
    self->unknown180 += 2;
    if (uint32_t(self->unknown180) <= staticFrames && !still) {
        return;
    }
    Virtual<void>(self, 0x1C, 1);   // (SetIsStatic)
    self->moveSpeed = vec0;
    self->turnSpeed = vec0;
}

// 0x4BA4E0: its physics' per-frame reset; leaning (+0x6B8 2): +0x6A4 grows to 20
// degrees, +0x6AC / +0x6A8 follow the matrix's +8 (its tilt) plus 15 degrees; else the
// matrix turned by the steering
void __fastcall Bike_4BA4E0(CBike* self) {
    self->flags33B &= 0xF5;
    At<int32_t>(self, 0x2C) = 0;
    At<int32_t>(self, 0x38) = 0;
    At<int32_t>(self, 0x40) = 0;
    At<int32_t>(self, 0x7C) = 0;
    At<uint8_t>(self, 0x181) = 0;
    At<int32_t>(self, 0x4C) = 0;
    Physical_468DF0(self);
    bool leaning = (At<uint8_t>(self, 0x6B8) >> 1) & 1;
    if (leaning && At<float>(self, 0x6A4) < 0.3490658601125081) {
        At<float>(self, 0x6A4) = float(timeStep * 0.017453293005625408 + At<float>(self, 0x6A4));
    }
    if (!leaning) {
        float steer = self->steerAngle;
        Matrix_413220(Entity_GetMatrix(self), steer);
        return;
    }
    float k = float(std::pow(0.9700000286102295, double(timeStep)));
    float tilt = Entity_GetMatrix(self)[2];
    if (tilt > 1.0) {
        tilt = 1.0f;
    }
    if (tilt < -1.0) {
        tilt = -1.0f;
    }
    double kept = k * At<float>(self, 0x6AC);
    float angle = float(std::asin(double(tilt)));
    float v = float(kept - (angle + 0.2617993950843811) * (1.0f - k));
    At<float>(self, 0x6AC) = v;
    At<float>(self, 0x6A8) = v;
}

// 0x4BA620 (an argument, unused): +0x6A4 grows to 20 degrees while leaning (+0x6B8 2,
// not 1) and not static; else it's the steering, or decays when at rest
void __fastcall Bike_4BA620(CBike* self, re::Edx, int) {
    uint8_t flags = At<uint8_t>(self, 0x6B8);
    if ((self->typeAndStatus & 0xF8) && (flags & 2) && !(flags & 1)) {
        if (At<float>(self, 0x6A4) < 0.3490658601125081) {
            At<float>(self, 0x6A4) = float(timeStep * 0.02617993950843811 + At<float>(self, 0x6A4));
        }
        return;
    }
    if (0.01f > std::fabs(self->moveSpeed.x) && 0.01f > std::fabs(self->moveSpeed.y) && self->steerAngle == 0.0f) {
        float k = float(std::pow(0.9599999785423279, double(timeStep)));
        At<float>(self, 0x6A4) = k * At<float>(self, 0x6A4);
        return;
    }
    At<float>(self, 0x6A4) = self->steerAngle;
}

// 0x4BA700: the steering (vf56) turned by half a degree a step towards the side it
// leans to, up to 25 degrees (not while +0x6B8 has 0x13)
void __fastcall Bike_4BA700(CBike* self) {
    if (At<uint8_t>(self, 0x6B8) & 0x13) {
        return;
    }
    float tilt = Entity_GetMatrix(self)[2];
    float steer = self->steerAngle;
    if (!(0.0f <= tilt)) {
        if (!(steer <= -0.4363323251406352)) {
            Virtual<void>(self, 0xE0, float(steer - timeStep * 0.008726646502812704));
        }
        return;
    }
    if (steer < 0.4363323251406352) {
        Virtual<void>(self, 0xE0, float(timeStep * 0.008726646502812704 + self->steerAngle));
    }
}


// a matrix row, or what it is without a matrix
CVector RowOr(CBike* self, int row, CVector none) {
    float* m = re::PtrCast<float*>(self->matrix);
    if (!m) {
        return none;
    }
    return {m[row * 4], m[row * 4 + 1], m[row * 4 + 2]};
}

CVector Right(CBike* self) {
    CVector r;
    if (float* m = re::PtrCast<float*>(self->matrix)) {
        r = {m[0], m[1], m[2]};
    } else {
        Placement_GetRight(&self->placementPos, &r);
    }
    return r;
}

// 0x4BA7A0: the player's bike's balance: trails when both wheels are off the ground,
// and turn forces for the wheelie (+0x6B0, while +0x709) and the lean (+0x6B4)
// against its turn speed (the bike data at +0x5D4: +0x48..+0x54, the tuning's +0x68..+0x74)
void __fastcall Bike_ProcessBalance(CBike* self) {
    if (!Seats_IsDriver(Seats(self), playerPed)) {
        return;
    }
    bool onGround = false;
    for (int i = 0; i < 2; i++) {
        if (1.0f > At<float>(self, 0x62C + i * 4)) {
            onGround = true;
            break;
        }
    }
    Game_SetTrails(self, !onGround);
    bool isPlayers = self == FindPlayerVehicle(0);
    uint8_t mode = At<uint8_t>(self, 0x6EC);
    if (mode && !At<uint8_t>(self, 0x780)) {
        bool keep = false;
        if (mode == 1) {
            CVector up = RowOr(self, 2, {0.0f, 0.0f, 1.0f});
            float dot = At<float>(self, 0x5A4) * up.y + At<float>(self, 0x5A0) * up.x + At<float>(self, 0x5A8) * up.z;
            keep = dot < 0.707f;
        }
        if (!keep) {
            At<uint8_t>(self, 0x709) = 0;
            return;
        }
    }
    uint8_t* data = At<uint8_t*>(self, 0x5D4);
    float* wheelie = &At<float>(data, 0x54);
    float* lean = &At<float>(data, 0x50);
    if (At<uint8_t>(self, 0x708) || At<uint8_t>(self, 0x780) || (At<int32_t>(self, 0x4C) != 0 && !mode)) {
        if (At<float>(data, 0x54) < At<float>(data, 0x4C)) {
            wheelie = &At<float>(data, 0x4C);
            lean = &At<float>(data, 0x48);
        }
    }
    CVector tmp, tmp2;
    if (isPlayers) {
        float w = At<float>(self, 0x6B0);
        if (w < 0.10000000149011612 && -1.1f < w) {
            At<uint8_t>(self, 0x709) = 1;
        }
        if (At<uint8_t>(self, 0x709)) {
            float a = At<float>(self, 0x6B0) * *wheelie;
            CVector r = Right(self);
            float dot = self->turnSpeed.y * r.y + self->turnSpeed.x * r.x + self->turnSpeed.z * r.z;
            float v = (a - -dot) * At<float>(self, 0x158);
            float k = At<float>(self, 0x6B0) == 0.0f ? At<float>(bikeTuning, 0x74) : At<float>(bikeTuning, 0x70);
            v = k * v;
            CVector up = RowOr(self, 2, {0.0f, 0.0f, 1.0f});
            float f = -v * timeStep;
            CVector force = {up.x * f, up.y * f, up.z * f};
            const CVector* p = VectorAdd(&tmp2, &self->centreOfMass, Entity_4154A0(self, &tmp));
            Physical_ApplyTurnForce(self, force.x, force.y, force.z, p->x, p->y, p->z);
        }
    }
    float b = At<float>(self, 0x6B4) * *lean;
    CVector up = RowOr(self, 2, {0.0f, 0.0f, 1.0f});
    float dot = self->turnSpeed.y * up.y + self->turnSpeed.x * up.x + self->turnSpeed.z * up.z;
    float v = (b - dot) * At<float>(self, 0x158);
    float k = At<float>(self, 0x6B4) == 0.0f ? At<float>(bikeTuning, 0x6C) : At<float>(bikeTuning, 0x68);
    v = k * v;
    CVector forward;
    if (self->matrix) {
        forward = RowOr(self, 1, {});
    } else {
        float heading = At<float>(self, 0x10);
        forward = {-float(std::sin(double(heading))), float(std::cos(double(heading))), 0.0f};
    }
    CVector r = Right(self);
    float f = -v * timeStep;
    CVector force = {r.x * f, r.y * f, r.z * f};
    const CVector* p = VectorAdd(&tmp, &self->centreOfMass, &forward);
    Physical_ApplyTurnForce(self, force.x, force.y, force.z, p->x, p->y, p->z);
}


// the driver's +0x1310 is 0xD (a kind of ped: the player's own control?)
bool DriverIs0xD(CBike* self) {
    return At<int32_t>(Seats_GetOccupant(Seats(self), 0), 0x1310) == 0xD;
}

bool PlayerHas1ED8() {
    return *At<uint8_t*>(playerPed, 0x1ED8) != 0;
}

uint8_t PlayerInput(void* control) {
    return Input_5F4800(At<uint8_t*>(playerPed, 0x2E0) + 0x258, control, 1, 1);
}

// 0x4BAC00: pedalling: +0x470 the push left (down by 0.025 a step, less when fast),
// +0x474 pedalling (a driver 0xD: 0x738840(6) and the pedal control), +0x478 when;
// the push as move forces (forward, against its speed) and a turn force from the
// steering; a quick hop when the pedalling stops (0x738900(7), within 250 ms)
void __fastcall Bike_ProcessPedalling(CBike* self) {
    if (!Seats_GetOccupant(Seats(self), 0) || !Ped_IsSeatedInVehicle(Seats_GetOccupant(Seats(self), 0))) {
        At<float>(self, 0x470) = 0.0f;
        At<uint8_t>(self, 0x474) = 0;
        return;
    }
    CVector& speed = self->moveSpeed;
    float speedSqr = speed.y * speed.y + speed.x * speed.x + speed.z * speed.z;
    float slow = float(1.2000000476837158 - speedSqr / 0.15000000596046448);
    if (slow < 0.6499999761581421) {
        slow = 0.65f;
    }
    if (slow > 1.0) {
        slow = 1.0f;
    }
    uint8_t wasPedalling = At<uint8_t>(self, 0x474);
    float& push = At<float>(self, 0x470);
    push = float(push - timeStep * 0.02500000037252903 * slow);
    if (DriverIs0xD(self)) {
        uint8_t pedalling = 0;
        if (Fn_738840(6, 0) > 0.0f && !PlayerHas1ED8() &&
            (1.0f > At<float>(self, 0x634) || 1.0f > At<float>(self, 0x638)) && !PlayerInput(controlBikeJump)) {
            pedalling = 1;
        }
        At<uint8_t>(self, 0x474) = pedalling;
    }
    CVector tmp, tmp2, tmp3;
    if (!(0.0f < push)) {
        push = 0.0f;
        float lim = At<float>(bikeTuning, 0x78);
        bool hop;
        if (!At<uint8_t>(self, 0x474)) {
            hop = true;
        } else {
            bool slowEnough = wasPedalling != 0;
            if (!slowEnough) {
                float sqr = speed.x * speed.x + speed.y * speed.y;
                slowEnough = !(float(lim * lim) < sqr);
            }
            hop = slowEnough && DriverIs0xD(self);
        }
        if (!hop) {
            push = 1.0f;
            At<uint32_t>(self, 0x478) = timeInMs;
            return;
        }
        int32_t cls;
        if (DriverIs0xD(self) && Fn_738900(7, 0) && At<uint32_t>(self, 0x478) + 0xFA > timeInMs &&
            *Vehicle_GetVehicleClass(self, &cls) != 8 && !PlayerHas1ED8() &&
            (1.0f > At<float>(self, 0x634) || 1.0f > At<float>(self, 0x638)) && !PlayerInput(controlBikeJump)) {
            float mass = At<float>(self, 0x158);
            At<uint32_t>(self, 0x478) = 0;
            const CVector* up = VectorScale(&tmp2, Entity_4154A0(self, &tmp), 0.12f);
            Physical_ApplyMoveForce(self, VectorScale(&tmp3, up, mass));
        }
        return;
    }
    At<uint32_t>(self, 0x478) = timeInMs;
    if (!At<uint8_t>(self, 0x474)) {
        push = 0.0f;
        return;
    }
    if (!(1.0f > At<float>(self, 0x634)) && !(1.0f > At<float>(self, 0x638))) {
        return;
    }
    float mass = At<float>(self, 0x158);
    float fast = float(Vector_MagnitudeSqr(&speed) / 0.20000000298023224);
    if (fast < 0.5) {
        fast = 0.5f;
    }
    if (fast > 1.0) {
        fast = 1.0f;
    }
    float step = timeStep;
    const CVector* forward = Entity_4154A0(self, &tmp);
    float along = float(1.149999976158142 - DotProduct(Normalised(&tmp2, &speed), forward));
    if (along < 0.0) {
        along = 0.0f;
    }
    float power = float(double(mass) * 0.09000000357627869 * fast * step * 0.5 * along);
    int32_t cls;
    if (DriverIs0xD(self) && Fn_738840(7, 0) == 0.0f) {
        power = float(power * 0.5);
    }
    if (*Vehicle_GetVehicleClass(self, &cls) == 8) {
        power = float(power * 0.5);
    }
    // against its speed
    const CVector* dir = Normalised(&tmp2, &speed);
    CVector back = {-dir->x, -dir->y, -dir->z};
    CVector f = {back.x * power, back.y * power, back.z * power};
    float k = 1.0f - push;
    CVector force = {f.x * k, f.y * k, f.z * k};
    Physical_ApplyMoveForce(self, &force);
    // forwards
    forward = Entity_4154A0(self, &tmp);
    f = {forward->x * power, forward->y * power, forward->z * power};
    k = float(1.0 - push * 0.6700000166893005);
    force = {f.x * k, f.y * k, f.z * k};
    Physical_ApplyMoveForce(self, &force);
    // the turn from the steering (+0x6C4 a phase)
    step = timeStep;
    float grip = float(Vector_MagnitudeSqr(&speed) / 0.05000000074505806 + 0.5);
    if (grip > 1.2000000476837158) {
        grip = 1.2f;
    }
    float* m = Entity_GetMatrix(self);
    float phase = float(std::sin(double(At<float>(self, 0x6C4))));
    float r = float((phase + push) * -self->steerAngle * At<float>(self, 0x15C) * 0.01600000075995922);
    CVector t = {m[0] * r, m[1] * r, m[2] * r};
    t = {t.x * grip, t.y * grip, t.z * grip};
    t = {t.x * step, t.y * step, t.z * step};
    t = {float(t.x * 0.5), float(t.y * 0.5), float(t.z * 0.5)};
    forward = Entity_4154A0(self, &tmp);
    float dot = DotProduct(Normalised(&tmp2, &speed), forward);
    CVector turn = {t.x * dot, t.y * dot, t.z * dot};
    const CVector* pos = self->matrix ? reinterpret_cast<const CVector*>(&self->matrix->m[12]) : &self->placementPos;
    Physical_45D410(self, VectorSub(&tmp3, &At<CVector>(self, 0x604), pos), &turn);
}


// 0x4BB330: each wheel's compression (+0x62C) kept as the last (+0x634) and reset to 1
// (a rumble strength from the compression change and, for the player's bike, the
// surface is worked out and not used: the PC build dropped it)
void __fastcall Bike_UpdateWheelsLast(CBike* self) {
    for (int i = 0; i < 2; i++) {
        FindPlayerVehicle(0);
        At<float>(self, 0x634 + i * 4) = At<float>(self, 0x62C + i * 4);
        At<float>(self, 0x62C + i * 4) = 1.0f;
    }
}

// vf42 0x4BB470: the rider's controls: the player's from the pad (0x738840: 0x10 the
// lean, 0x11 the wheelie, 0x12 added unless 0x738500 says so), else from the autopilot
// (0x4CCC00) scaled by the driver's +0x2E0's +0x1C; dead zones, then within -1..1;
// when 0x50DF50 says no, braking and its speed kept within 0.28
void __fastcall Bike_ProcessControlInputs(CBike* self) {
    if (!Seats_GetOccupant(Seats(self), 0)) {
        return;
    }
    self->flags33A &= 0xFB;
    self->unknown368 = 0;
    float lean, wheelie;
    if (Seats_IsDriver(Seats(self), playerPed)) {
        if (flagA14925 && !Fn_738500(Fn_738070())) {
            double extra = Fn_738840(0x12, 0);
            lean = float(Fn_738840(0x10, 0) + extra);
            wheelie = Fn_738840(0x11, 0);
        } else {
            lean = Fn_738840(0x10, 0);
            wheelie = Fn_738840(0x11, 0);
        }
    } else {
        CVector dir;
        Vehicle_4CCC00(self, &dir);
        float k = At<float>(At<uint8_t*>(Seats_GetOccupant(Seats(self), 0), 0x2E0), 0x1C);
        dir.x = k * dir.x;
        dir.y = k * dir.y;
        lean = -dir.x;
        wheelie = dir.y;
    }
    At<float>(self, 0x6B4) = lean;
    At<float>(self, 0x6B0) = wheelie;
    if (At<float>(bikeTuning, 0x60) > std::fabs(lean)) {
        At<float>(self, 0x6B4) = 0.0f;
    }
    if (At<float>(bikeTuning, 0x64) > std::fabs(wheelie)) {
        At<float>(self, 0x6B0) = 0.0f;
    }
    float v = At<float>(self, 0x6B4);
    At<float>(self, 0x6B4) = v <= -1.0f ? -1.0f : (v >= 1.0f ? 1.0f : v);
    v = At<float>(self, 0x6B0);
    At<float>(self, 0x6B0) = v <= -1.0f ? -1.0f : (v >= 1.0f ? 1.0f : v);
    if (Fn_50DF50(objA45A30)) {
        return;
    }
    self->flags33A |= 4;
    At<float>(self, 0x298) = 1.0f;
    At<float>(self, 0x294) = float(double(self->unknown29C) * 0.0);
    CVector& speed = self->moveSpeed;
    float sqr = speed.y * speed.y + speed.x * speed.x + speed.z * speed.z;
    float length = float(std::sqrt(double(std::fabs(sqr))));
    if (length > 0.28f) {
        float k = float(0.2800000011920929 / length);
        speed = {k * speed.x, speed.y * k, k * speed.z};
    }
}

// vf56 0x4BB760: the steering towards an angle, by the tuning's rate a step (+0x38 when
// turning further the same way, else +0x3C)
void __fastcall Bike_SetSteerAngle(CBike* self, re::Edx, float angle) {
    float steer = self->steerAngle;
    float rate;
    if (!(double(angle * steer) < 0.0) && !(std::fabs(steer) > std::fabs(angle))) {
        rate = At<float>(bikeTuning, 0x38);
    } else {
        rate = At<float>(bikeTuning, 0x3C);
    }
    rate = rate * timeStep;
    float diff = std::fabs(angle - self->steerAngle);
    if (!(rate < diff)) {
        self->steerAngle = angle;
    } else if (self->steerAngle < angle) {
        self->steerAngle = rate + self->steerAngle;
    } else {
        self->steerAngle = self->steerAngle - rate;
    }
}

// vf50 0x4BB820: a wheel (0 or 1) burst: once, (a parked bike made status 3) and with
// `jolt` random move and turn forces; the rider thrown off when fast (for pieces 0xD /
// 0xE: never reached, the pieces past 1 were turned away first)
void __fastcall Bike_BurstTyre(CBike* self, re::Edx, int pieceArg, int jolt) {
    if (self->flags33B & 0x10) {
        return;
    }
    uint8_t piece = uint8_t(pieceArg);
    if (piece >= 2) {
        return;
    }
    if (piece == 0xD) {
        piece = 0;
    } else if (piece == 0xF) {
        piece = 1;
    }
    uint8_t& burst = At<uint8_t>(self, 0x5D8 + piece);
    if (!burst) {
        burst = 1;
        if ((self->typeAndStatus & 0xF8) == 0x10) {
            Virtual<void>(self, 0x18, 3);
        }
        if (re::Flag(jolt)) {
            CVector tmp;
            float* m = Entity_GetMatrix(self);
            float f = RandomFloatInRange(-0.02f, 0.02f) * At<float>(self, 0x158);
            CVector force = {m[0] * f, m[1] * f, m[2] * f};
            Physical_ApplyMoveForce(self, &force);
            m = Entity_GetMatrix(self);
            f = RandomFloatInRange(-0.02f, 0.02f) * At<float>(self, 0x15C);
            CVector turn = {m[0] * f, m[1] * f, m[2] * f};
            Physical_45D410(self, &turn, Entity_4154A0(self, &tmp));
        }
    }
    if (!Seats_GetOccupant(Seats(self), 0)) {
        return;
    }
    if (piece == 0xD) {
        if (!(1.0f > At<float>(self, 0x634))) {
            return;
        }
    } else if (piece != 0xE || !(1.0f > At<float>(self, 0x638))) {
        return;
    }
    float speed = Vector_Magnitude(&self->moveSpeed);
    if (speed > 0.3f && speed > 0.55f && piece == 0xD) {
        Game_KnockOffRider(self, -5, nullptr, Seats_GetOccupant(Seats(self), 0), 0);
    }
}

// 0x4BBA40: the surface sound for the wheels (in water: +0x5E4 / +0x60C below 0, or
// grass 3, dirt 2, sand 0x1B), the rear wheel's last; left as it is for others
void __fastcall Bike_GetSurfaceSound(CBike* self, re::Edx, int32_t* out) {
    static const int wheels[2][2] = {{0x5E4, 0x5FE}, {0x60C, 0x626}};
    for (const auto& wheel : wheels) {
        if (0.0f > At<float>(self, wheel[0])) {
            *out = soundBikeOnBigWet;
            continue;
        }
        uint8_t surface = At<uint8_t>(self, wheel[1]);
        if (surface == 3) {
            *out = soundBikeOnGrass;
        } else if (surface == 2) {
            *out = soundBikeOnDirt;
        } else if (surface == 0x1B) {
            *out = soundBikeOnSand;
        }
    }
}

// 0x4BBAD0: the two wheels' contact points (+0x5DC the front, +0x604 the rear)
void __fastcall Bike_GetWheelPoints(CBike* self, re::Edx, CVector* front, CVector* rear) {
    *rear = At<CVector>(self, 0x604);
    *front = At<CVector>(self, 0x5DC);
}


// 0x4BBB10: the wheels' surface sounds, louder with speed (or in water, where
// 0x430320 says the area has it); none in the snowy chapter's such areas
void __fastcall Bike_UpdateSurfaceSounds(CBike* self) {
    Game_LoadSounds();
    uint8_t water = Fn_430320(currArea);
    if (chapterAreaKinds[chapter] == 2 && water == 1) {
        return;
    }
    CVector rear = At<CVector>(self, 0x604);
    CVector front = At<CVector>(self, 0x5DC);
    CVector& speed = self->moveSpeed;
    float sqr = speed.y * speed.y + speed.x * speed.x + speed.z * speed.z;
    int32_t id = -1;
    float s = float(sqr * 100.0);
    bool inWater = water == 1 && (0.0f > front.z || 0.0f > rear.z);
    if (!(s > 1.0f) && !inWater) {
        Game_StopSounds(self);
        return;
    }
    float volume = inWater ? float((s - 0.0f) / 1.0f) : float((s - 1.0) / 3.0);
    if (volume < 0.0f) {
        volume = 0.0f;
    } else if (volume > 1.0f) {
        volume = 1.0f;
    }
    Game_GetSurfaceSound(self, &id);
    Game_StopSounds(self);
    if (id == -1) {
        return;
    }
    uint32_t key = uint16_t(id);
    SoundManager_Play(soundManager, &key, &At<uint8_t>(self, 0x6F0), 0);
    key = uint16_t(id);
    SoundManager_Play(soundManager, &key, &At<uint8_t>(self, 0x6F8), 0);
    if (At<int32_t>(self, 0x6F4) != 0) {
        void* sound = &At<uint8_t>(self, 0x6F0);
        Sound_SetPosition(sound, &front, 0);
        Sound_SetVolume(sound, volume);
        Sound_SetPitch(sound, volume);
    }
    if (At<int32_t>(self, 0x6FC) != 0) {
        void* sound = &At<uint8_t>(self, 0x6F8);
        Sound_SetPosition(sound, &rear, 0);
        Sound_SetVolume(sound, volume);
        Sound_SetPitch(sound, volume);
    }
}

} // namespace

// ---- registration ---------------------------------------------------------------------

#define BIKEC_REVERSED(name, address, fn)                                                    \
    static void* RE_CONCAT(Original_, __LINE__);                                             \
    REVERSED(name, address, RE_BODY(fn), nullptr, &RE_CONCAT(Original_, __LINE__), re::Mode::Original)

BIKEC_REVERSED("CBikeSpring::Stop", 0x004B9460, &Spring_Stop);
BIKEC_REVERSED("CBikeSpring::Reset", 0x004B9470, &Spring_Reset);
BIKEC_REVERSED("CBikeSpring::Update", 0x004B94E0, &Spring_Update);
BIKEC_REVERSED("CBike::SetTrails", 0x004B95C0, &Bike_SetTrails);
BIKEC_REVERSED("CBike::sub_4B96C0", 0x004B96C0, &Bike_4B96C0);
BIKEC_REVERSED("CBike::sub_4B9740", 0x004B9740, &Bike_4B9740);
BIKEC_REVERSED("CBike::UpdateSteering", 0x004B9800, &Bike_UpdateSteering);
BIKEC_REVERSED("CBike::GetUnknownVf57", 0x004B9A30, &Bike_GetUnknownVf57);
BIKEC_REVERSED("CBike::UpdateHorn", 0x004B9AB0, &Bike_UpdateHorn);
BIKEC_REVERSED("CBike::LoadSounds", 0x004B9B00, &Bike_LoadSounds);
BIKEC_REVERSED("CBike::StopSounds", 0x004B9BA0, &Bike_StopSounds);
BIKEC_REVERSED("CBike::GetTopSpeed", 0x004B9C30, &Bike_GetTopSpeed);
BIKEC_REVERSED("CBike::PlayAnim", 0x004B9CE0, &Bike_PlayAnim);
BIKEC_REVERSED("CBike::PlayBaseAnim", 0x004B9D40, &Bike_PlayBaseAnim);
BIKEC_REVERSED("CBike::sub_4B9E10", 0x004B9E10, &Bike_4B9E10);
BIKEC_REVERSED("CBike::Destruct", 0x004B9E80, &Bike_Destruct);
BIKEC_REVERSED("CBike::IsA", 0x004B9F80, &Bike_IsA);
BIKEC_REVERSED("CBike::KnockOffRider", 0x004BA050, &Bike_KnockOffRider);
BIKEC_REVERSED("CBike::CheckStatic", 0x004BA220, &Bike_CheckStatic);
BIKEC_REVERSED("CBike::sub_4BA4E0", 0x004BA4E0, &Bike_4BA4E0);
BIKEC_REVERSED("CBike::sub_4BA620", 0x004BA620, &Bike_4BA620);
BIKEC_REVERSED("CBike::sub_4BA700", 0x004BA700, &Bike_4BA700);
BIKEC_REVERSED("CBike::ProcessBalance", 0x004BA7A0, &Bike_ProcessBalance);
BIKEC_REVERSED("CBike::ProcessPedalling", 0x004BAC00, &Bike_ProcessPedalling);
BIKEC_REVERSED("CBike::UpdateWheelsLast", 0x004BB330, &Bike_UpdateWheelsLast);
BIKEC_REVERSED("CBike::ProcessControlInputs", 0x004BB470, &Bike_ProcessControlInputs);
BIKEC_REVERSED("CBike::SetSteerAngle", 0x004BB760, &Bike_SetSteerAngle);
BIKEC_REVERSED("CBike::BurstTyre", 0x004BB820, &Bike_BurstTyre);
BIKEC_REVERSED("CBike::GetSurfaceSound", 0x004BBA40, &Bike_GetSurfaceSound);
BIKEC_REVERSED("CBike::GetWheelPoints", 0x004BBAD0, &Bike_GetWheelPoints);
BIKEC_REVERSED("CBike::UpdateSurfaceSounds", 0x004BBB10, &Bike_UpdateSurfaceSounds);
